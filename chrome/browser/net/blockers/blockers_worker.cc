/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "blockers_worker.h"
#include <fstream>
#include <sstream>
#include "../../../../base/android/apk_assets.h"
#include "../../../../content/public/common/resource_type.h"
#include "../../../../base/files/file_util.h"
#include "../../../../base/path_service.h"
#include "../../../../base/json/json_reader.h"
#include "../../../../base/values.h"
#include "../../../../third_party/sqlite/sqlite3.h"
#include "../../../../third_party/re2/src/re2/re2.h"
#include "../../../../url/gurl.h"
#include "TPParser.h"
#include "ABPFilterParser.h"

#define TP_DATA_FILE        "TrackingProtectionDownloaded.dat"
#define ADBLOCK_DATA_FILE   "ABPFilterParserDataDownloaded.dat"
#define HTTPSE_DATA_FILE    "httpseDownloaded.sqlite"

namespace net {
namespace blockers {

    int SQLITEIdsCallback(void *a_param, int argc, char **argv, char **column) {
      if (argc <= 0 || nullptr == argv) {
          return 0;
      }
      std::string* ruleIds = (std::string*)a_param;
      if (0 != ruleIds->length()) {
          *ruleIds += ",";
      }
      std::string toInsert(argv[0]);
      if (toInsert.length() >= 2 && toInsert[0] == '[' && toInsert[toInsert.length() - 1] == ']') {
        toInsert.erase(0, 1);
        toInsert.erase(toInsert.length() - 1);
      }
      *ruleIds += toInsert;

      return 0;
    }

    int SQLITECallback(void *a_param, int argc, char **argv, char **column) {
      if (argc <= 0 || nullptr == argv) {
          return 0;
      }
      std::vector<std::string>* rules = (std::vector<std::string>*)a_param;
      rules->push_back(argv[0]);

      return 0;
    }


    BlockersWorker::BlockersWorker() :
        httpse_db_(nullptr),
        tp_parser_(nullptr),
        adblock_parser_(nullptr) {
        base::ThreadRestrictions::SetIOAllowed(true);
    }

    BlockersWorker::~BlockersWorker() {
        if (nullptr != tp_parser_) {
            delete tp_parser_;
        }
        if (nullptr != adblock_parser_) {
            delete adblock_parser_;
        }
        if (nullptr != httpse_db_) {
            sqlite3_close(httpse_db_);
        }
    }

    bool BlockersWorker::InitAdBlock() {
        std::lock_guard<std::mutex> guard(adblock_init_mutex_);

        if (!GetData(ADBLOCK_DATA_FILE, adblock_buffer_)) {
            return false;
        }

        adblock_parser_ = new ABPFilterParser();
        adblock_parser_->deserialize((char*)&adblock_buffer_.front());

        return false;
    }

    bool BlockersWorker::InitTP() {
        std::lock_guard<std::mutex> guard(tp_init_mutex_);

        if (!GetData(TP_DATA_FILE, tp_buffer_)) {
            return false;
        }

        tp_parser_ = new CTPParser();
        tp_parser_->deserialize((char*)&tp_buffer_.front());

        return true;
    }

    bool BlockersWorker::InitHTTPSE() {
        std::lock_guard<std::mutex> guard(httpse_init_mutex_);

        // Init sqlite database
        std::vector<unsigned char> db_file_name;
        if (!GetData(HTTPSE_DATA_FILE, db_file_name, true)) {
            return false;
        }
        base::FilePath app_data_path;
        PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);
        base::FilePath dbFilePath = app_data_path.Append((char*)&db_file_name.front());
        int err = sqlite3_open(dbFilePath.value().c_str(), &httpse_db_);
        if (err != SQLITE_OK) {
            LOG(ERROR) << "sqlite db open error " << dbFilePath.value().c_str() << ", err == " << err;

            return false;
        }

        return true;
    }

    bool BlockersWorker::GetData(const char* fileName, std::vector<unsigned char>& buffer, bool only_file_name) {
        base::FilePath app_data_path;
        PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);

        base::FilePath dataFilePathDownloaded = app_data_path.Append(fileName);
        int64_t size = 0;
        if (!base::PathExists(dataFilePathDownloaded)
            || !base::GetFileSize(dataFilePathDownloaded, &size)
            || 0 == size) {
            LOG(ERROR) << "GetData: the dat info file is corrupted: " << fileName;

            return false;
        }
        std::vector<char> data(size + 1);
        if (size != base::ReadFile(dataFilePathDownloaded, (char*)&data.front(), size)) {
            LOG(ERROR) << "BlockersWorker::InitTP: cannot read dat info file " << fileName;

            return false;
        }
        data[size] = '\0';
        if (only_file_name) {
            buffer.resize(size + 1);
            ::memcpy(&buffer.front(), &data.front(), size + 1);

            return true;
        }

        base::FilePath dataFilePath = app_data_path.Append(&data.front());
        if (!base::PathExists(dataFilePath)
            || !base::GetFileSize(dataFilePath, &size)
            || 0 == size) {
            LOG(ERROR) << "BlockersWorker::InitTP: the dat file is corrupted " << &data.front();

            return false;
        }
        buffer.resize(size);
        if (size != base::ReadFile(dataFilePath, (char*)&buffer.front(), size)) {
            LOG(ERROR) << "BlockersWorker::InitTP: cannot read dat file " << &data.front();

            return false;
        }

        return true;
    }

    bool BlockersWorker::shouldAdBlockUrl(const std::string& base_host, const std::string& url, unsigned int resource_type) {
        if (nullptr == adblock_parser_ && !InitAdBlock()) {
            return false;
        }

        FilterOption currentOption = FONoFilterOption;
        content::ResourceType internalResource = (content::ResourceType)resource_type;
        if (content::RESOURCE_TYPE_STYLESHEET == internalResource) {
            currentOption = FOStylesheet;
        } else if (content::RESOURCE_TYPE_IMAGE == internalResource) {
            currentOption = FOImage;
        } else if (content::RESOURCE_TYPE_SCRIPT == internalResource) {
            currentOption = FOScript;
        }

        if (adblock_parser_->matches(url.c_str(), currentOption, base_host.c_str())) {
            return true;
        }

        return false;
    }

    bool BlockersWorker::shouldTPBlockUrl(const std::string& base_host, const std::string& host) {
        if (nullptr == tp_parser_ && !InitTP()) {
            return false;
        }

        if (!tp_parser_->matchesTracker(base_host.c_str(), host.c_str())) {
            return false;
        }

        char* thirdPartyHosts = tp_parser_->findFirstPartyHosts(base_host.c_str());
        std::vector<std::string> hosts;
        if (nullptr != thirdPartyHosts) {
             std::string strThirdPartyHosts = thirdPartyHosts;
             size_t iPos = strThirdPartyHosts.find(",");
             while (iPos != std::string::npos) {
                 std::string thirdParty = strThirdPartyHosts.substr(0, iPos);
                 strThirdPartyHosts = strThirdPartyHosts.substr(iPos + 1);
                 iPos = strThirdPartyHosts.find(",");
                 hosts.push_back(thirdParty);
            }
            if (0 != strThirdPartyHosts.length()) {
              hosts.push_back(strThirdPartyHosts);
            }
            delete []thirdPartyHosts;
        }

        for (size_t i = 0; i < hosts.size(); i++) {
            if (host == hosts[i] || host.find((std::string)"." + hosts[i]) != std::string::npos) {
               return false;
            }
            size_t iPos = host.find((std::string)"." + hosts[i]);
            if (iPos == std::string::npos) {
                continue;
            }
            if (hosts[i].length() + ((std::string)".").length() + iPos == host.length()) {
                return false;
            }
        }

        // That is just temporarily, we will have to figure that out
        // inside the tracking protection lib
        std::vector<std::string> whiteList;
        whiteList.push_back("connect.facebook.net");
        whiteList.push_back("connect.facebook.com");
        whiteList.push_back("staticxx.facebook.com");
        whiteList.push_back("www.facebook.com");
        whiteList.push_back("scontent.xx.fbcdn.net");
        whiteList.push_back("pbs.twimg.com");
        whiteList.push_back("scontent-sjc2-1.xx.fbcdn.net");
        whiteList.push_back("platform.twitter.com");
        whiteList.push_back("syndication.twitter.com");
        for (size_t i = 0; i < whiteList.size(); i++) {
            if (whiteList[i] == host) {
                return false;
            }
        }

        return true;
    }

    std::string BlockersWorker::getHTTPSURL(const GURL* url) {
        if (nullptr == url
          || url->scheme() == "https"
          || (nullptr == httpse_db_ && !InitHTTPSE())) {
            return url->spec();
        }

        std::istringstream host(url->host());
        std::vector<std::string> domains;
        std::string domain;
        while (std::getline(host, domain, '.')) {
            domains.push_back(domain);
        }
        if (domains.size() <= 1) {
            return url->spec();
        }

        std::string query = "select ids from targets where host like '";
        std::string domain_to_check(domains[domains.size() - 1]);
        for (int i = domains.size() - 2; i >= 0; i--) {
            if (i != (int)domains.size() - 2) {
                query += " or host like '";
            }
            domain_to_check.insert(0, ".");
            domain_to_check.insert(0, domains[i]);
            std::string prefix;
            if (i > 0) {
                prefix = "*.";
            }
            query += prefix + domain_to_check + "'";
        }

        char *err = NULL;
        std::string ruleIds;
        if (SQLITE_OK != sqlite3_exec(httpse_db_, query.c_str(), SQLITEIdsCallback, &ruleIds, &err)) {
            LOG(ERROR) << "sqlite exec ids error: " << err;
            sqlite3_free(err);

            return "";
        }

        if (0 == ruleIds.length()) {
            return url->spec();
        }
        std::string newURL = getHTTPSNewHostFromIds(ruleIds, url->spec());
        if (0 != newURL.length()) {
            return newURL;
        }

        return url->spec();
    }

    std::string BlockersWorker::getHTTPSNewHostFromIds(const std::string& ruleIds, const std::string& originalUrl) {
        if (nullptr == httpse_db_) {
            return "";
        }

        std::vector<std::string> rules;
        std::string query("select contents from rulesets where id in (" + ruleIds + ")");
        char *err = NULL;
        if (SQLITE_OK != sqlite3_exec(httpse_db_, query.c_str(), SQLITECallback, &rules, &err)) {
            LOG(ERROR) << "sqlite exec error: " << err;
            sqlite3_free(err);

            return "";
        }

        std::string urlToCheck(originalUrl);
        if (0 != urlToCheck.length() && urlToCheck[urlToCheck.length() - 1] == '/') {
            urlToCheck.erase(urlToCheck.length() - 1);
        }
        for (int i = 0; i < (int)rules.size(); i++) {
            std::string newUrl(applyHTTPSRule(urlToCheck, rules[i]));
            if (0 != newUrl.length()) {
                return newUrl;
            }
        }

        return "";
    }

    std::string BlockersWorker::applyHTTPSRule(const std::string& originalUrl, const std::string& rule) {
        std::unique_ptr<base::Value> json_object = base::JSONReader::Read(rule);
        if (nullptr == json_object.get()) {
            LOG(ERROR) << "applyHTTPSRule: incorrect json rule";

            return "";
        }

        const base::DictionaryValue* dictionary_value = nullptr;
        json_object->GetAsDictionary(&dictionary_value);
        if (nullptr == dictionary_value) {
            LOG(ERROR) << "applyHTTPSRule: incorrect json rule content";

            return "";
        }

        if (dictionary_value->Get("ruleset.$.default_off", nullptr)
          || dictionary_value->Get("ruleset.$.platform", nullptr)) {
            return "";
        }

        // Check on exclusions
        const base::Value* exclusion = nullptr;
        if (dictionary_value->Get("ruleset.exclusion", &exclusion)) {
          const base::ListValue* values = nullptr;
          exclusion->GetAsList(&values);
          if (nullptr != values) {
            for (size_t i = 0; i < values->GetSize(); ++i) {
              const base::Value* child_value = nullptr;
              if (!values->Get(i, &child_value)) {
                continue;
              }
              const base::DictionaryValue* child_dictionary = nullptr;
              child_value->GetAsDictionary(&child_dictionary);
              if (nullptr == child_dictionary) {
                continue;
              }
              const base::Value* pattern_value = nullptr;
              if (!child_dictionary->Get("$.pattern", &pattern_value)) {
                continue;
              }
              std::string pattern;
              if (!pattern_value->GetAsString(&pattern)) {
                continue;
              }

              if (RE2::FullMatch(originalUrl, pattern)) {
                return "";
              }
            }
          }
        }

        // Apply pattern
        const base::Value* ruleToReplace = nullptr;
        if (dictionary_value->Get("ruleset.rule", &ruleToReplace)) {
          const base::ListValue* values = nullptr;
          ruleToReplace->GetAsList(&values);
          if (nullptr != values) {
            for (size_t i = 0; i < values->GetSize(); ++i) {
              const base::Value* child_value = nullptr;
              if (!values->Get(i, &child_value)) {
                continue;
              }
              const base::DictionaryValue* ruleToReplaceDictionary = nullptr;
              child_value->GetAsDictionary(&ruleToReplaceDictionary);
              if (nullptr == ruleToReplaceDictionary) {
                continue;
              }
              const base::Value* from_value = nullptr;
              const base::Value* to_value = nullptr;
              if (!ruleToReplaceDictionary->Get("$.from", &from_value)
                || !ruleToReplaceDictionary->Get("$.to", &to_value)) {
                continue;
              }
              std::string from, to;
              if (!from_value->GetAsString(&from)
                || !to_value->GetAsString(&to)) {
                continue;
              }
              std::string newUrl(originalUrl);
              RE2 regExp(from);

              if (RE2::Replace(&newUrl, regExp, to) && newUrl != originalUrl) {
                return newUrl;
              }
            }
          }
        }

        return "";
    }

}  // namespace blockers
}  // namespace net
