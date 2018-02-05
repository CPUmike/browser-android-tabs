/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_HELPER_H_
#define BAT_HELPER_H_

#include <string>
#include <vector>

#include "base/callback.h"

struct REQUEST_CREDENTIALS_ST {
  REQUEST_CREDENTIALS_ST();
  ~REQUEST_CREDENTIALS_ST();

  std::string proof_;
  std::string requestType_;
  std::string request_body_currency_;
  std::string request_body_label_;
  std::string request_body_publicKey_;
  std::string request_body_octets_;
  std::string request_headers_digest_;
  std::string request_headers_signature_;
};

struct WALLET_INFO_ST {
  WALLET_INFO_ST();
  ~WALLET_INFO_ST();

  std::string paymentId_;
  std::string addressBAT_;
  std::string addressBTC_;
  std::string addressCARD_ID_;
  std::string addressETH_;
  std::string addressLTC_;
  std::vector<uint8_t> keyInfoSeed_;
};

struct CLIENT_STATE_ST {
  CLIENT_STATE_ST();
  ~CLIENT_STATE_ST();

  WALLET_INFO_ST walletInfo_;
  uint64_t bootStamp_;
  uint64_t reconcileStamp_;
  std::string personaId_;
  std::string userId_;
  std::string registrarVK_;
  std::string masterUserToken_;
  std::string fee_currency_;
  std::string settings_;
  double fee_amount_;
  unsigned int days_;
};

struct PUBLISHER_STATE_ST {
  PUBLISHER_STATE_ST();
  ~PUBLISHER_STATE_ST();

  unsigned int min_pubslisher_duration_;  // In milliseconds
  unsigned int min_visits_;
  bool allow_non_verified_;
};

struct PUBLISHER_ST {
  PUBLISHER_ST();
  PUBLISHER_ST(const PUBLISHER_ST& publisher);
  ~PUBLISHER_ST();

  uint64_t duration_;
  std::string favicon_url_;
  double score_;
  unsigned int visits_;
  bool verified_;
  bool exclude_;
  bool pinPercentage_;
  uint64_t verifiedTimeStamp_;
  unsigned int percent_;
  bool deleted_;
};

struct PUBLISHER_DATA_ST {
  PUBLISHER_DATA_ST();
  PUBLISHER_DATA_ST(const PUBLISHER_DATA_ST& publisherData);
  ~PUBLISHER_DATA_ST();

  std::string publisherKey_;
  PUBLISHER_ST publisher_;
  unsigned int daysSpent_;
  unsigned int hoursSpent_;
  unsigned int minutesSpent_;
  unsigned int secondsSpent_;
};

struct FETCH_CALLBACK_EXTRA_DATA_ST {
  FETCH_CALLBACK_EXTRA_DATA_ST();
  ~FETCH_CALLBACK_EXTRA_DATA_ST();

  uint64_t value1;
  std::string string1;
  bool boolean1;
};

class BatHelper {
public:
  typedef base::Callback<void(bool, const std::string&, const FETCH_CALLBACK_EXTRA_DATA_ST&)> FetchCallback;
  typedef base::Callback<void(bool, const CLIENT_STATE_ST&)> ReadStateCallback;
  typedef base::Callback<void(bool, const PUBLISHER_STATE_ST&)> ReadPublisherStateCallback;

  static std::string getJSONValue(const std::string& fieldName, const std::string& json);
  static void getJSONWalletInfo(const std::string& json, WALLET_INFO_ST& walletInfo,
    std::string& fee_currency, double& fee_amount, unsigned int& days);
  static void getJSONState(const std::string& json, CLIENT_STATE_ST& state);
  static void getJSONPublisherState(const std::string& json, PUBLISHER_STATE_ST& state);
  static void getJSONPublisher(const std::string& json, PUBLISHER_ST& publisher_st);
  static void getJSONPublisherTimeStamp(const std::string& json, uint64_t& publisherTimestamp);
  static void getJSONPublisherVerified(const std::string& json, bool& verified);
  static std::vector<uint8_t> generateSeed();
  static std::vector<uint8_t> getHKDF(const std::vector<uint8_t>& seed);
  static void getPublicKeyFromSeed(const std::vector<uint8_t>& seed,
    std::vector<uint8_t>& publicKey, std::vector<uint8_t>& secretKey);
  static std::string uint8ToHex(const std::vector<uint8_t>& in);
  static std::string stringify(std::string* keys, std::string* values, const unsigned int& size);
  static std::string stringifyRequestCredentialsSt(const REQUEST_CREDENTIALS_ST& request_credentials);
  static std::string stringifyState(const CLIENT_STATE_ST& state);
  static std::string stringifyPublisherState(const PUBLISHER_STATE_ST& state);
  static std::string stringifyPublisher(PUBLISHER_ST& publisher_st);
  static std::vector<uint8_t> getSHA256(const std::string& in);
  static std::string getBase64(const std::vector<uint8_t>& in);
  static std::vector<uint8_t> getFromBase64(const std::string& in);
  // Sign using ed25519 algorithm
  static std::string sign(std::string* keys, std::string* values, const unsigned int& size,
    const std::string& keyId, const std::vector<uint8_t>& secretKey);
  static uint64_t currentTime();
  static void saveState(const CLIENT_STATE_ST& state);
  static void loadState(BatHelper::ReadStateCallback callback);
  static void savePublisherState(const PUBLISHER_STATE_ST& state);
  static void loadPublisherState(BatHelper::ReadPublisherStateCallback callback);
  // We have to implement different function for iOS, probably laptop
  static void writeStateFile(const std::string& data);
  // We have to implement different function for iOS, probably laptop
  static void readStateFile(BatHelper::ReadStateCallback callback);
  // We have to implement different function for iOS, probably laptop
  static void writePublisherStateFile(const std::string& data);
  // We have to implement different function for iOS, probably laptop
  static void readPublisherStateFile(BatHelper::ReadPublisherStateCallback callback);

private:
  BatHelper();
  ~BatHelper();
};

#endif  // BAT_HELPER_H_
