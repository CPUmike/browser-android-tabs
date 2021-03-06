// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/complete_offline_page_upgrade_task.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"
#include "components/offline_pages/core/test_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const char kContentsOfTempFile[] = "Sample content of temp file.";
const char kTargetFileName[] = "target_file_name.mhtml";
// TODO(fgorski): Replace test with actually computed digest.
const char kDummyDigest[] = "TestDigest==";

base::FilePath PrepareTemporaryFile(const base::FilePath& temp_dir) {
  base::FilePath temporary_file_path;
  EXPECT_TRUE(base::CreateTemporaryFileInDir(temp_dir, &temporary_file_path));
  EXPECT_TRUE(
      base::AppendToFile(temporary_file_path,
                         reinterpret_cast<const char*>(&kContentsOfTempFile[0]),
                         sizeof(kContentsOfTempFile)));
  return temporary_file_path;
}

}  // namespace

class CompleteOfflinePageUpgradeTaskTest : public testing::Test {
 public:
  CompleteOfflinePageUpgradeTaskTest();
  ~CompleteOfflinePageUpgradeTaskTest() override;

  void SetUp() override;
  void TearDown() override;
  OfflinePageItem CreateOfflinePage();

  void CompleteUpgradeDone(CompleteUpgradeStatus result);

  OfflinePageMetadataStoreSQL* store() { return store_test_util_.store(); }
  OfflinePageMetadataStoreTestUtil* store_test_util() {
    return &store_test_util_;
  }
  OfflinePageItemGenerator* generator() { return &generator_; }
  TestTaskRunner* runner() { return &runner_; }

  CompleteUpgradeCallback callback() {
    return base::BindOnce(
        &CompleteOfflinePageUpgradeTaskTest::CompleteUpgradeDone,
        base::Unretained(this));
  }

  const base::FilePath& temp_dir_path() const { return temp_dir_.GetPath(); }
  const base::FilePath& temporary_file_path() const {
    return temporary_file_path_;
  }
  const base::FilePath& target_file_path() const { return target_file_path_; }

  CompleteUpgradeStatus last_status() const { return last_status_; }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  OfflinePageMetadataStoreTestUtil store_test_util_;
  OfflinePageItemGenerator generator_;
  TestTaskRunner runner_;
  CompleteUpgradeStatus last_status_;
  base::ScopedTempDir temp_dir_;
  base::FilePath temporary_file_path_;
  base::FilePath target_file_path_;
};

CompleteOfflinePageUpgradeTaskTest::CompleteOfflinePageUpgradeTaskTest()
    : task_runner_(new base::TestMockTimeTaskRunner()),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_),
      runner_(task_runner_),
      last_status_(CompleteUpgradeStatus::DB_ERROR) {}

CompleteOfflinePageUpgradeTaskTest::~CompleteOfflinePageUpgradeTaskTest() {}

void CompleteOfflinePageUpgradeTaskTest::SetUp() {
  store_test_util_.BuildStoreInMemory();
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  generator()->SetArchiveDirectory(temp_dir_path());

  temporary_file_path_ = PrepareTemporaryFile(temp_dir_path());
  target_file_path_ = temp_dir_path().AppendASCII(kTargetFileName);
}

void CompleteOfflinePageUpgradeTaskTest::TearDown() {
  store_test_util_.DeleteStore();
}

OfflinePageItem CompleteOfflinePageUpgradeTaskTest::CreateOfflinePage() {
  OfflinePageItem page = generator()->CreateItemWithTempFile();
  page.upgrade_attempt = 3;
  store_test_util()->InsertItem(page);
  return page;
}

void CompleteOfflinePageUpgradeTaskTest::CompleteUpgradeDone(
    CompleteUpgradeStatus result) {
  last_status_ = result;
}

TEST_F(CompleteOfflinePageUpgradeTaskTest, Success) {
  OfflinePageItem original_page = CreateOfflinePage();

  auto task = std::make_unique<CompleteOfflinePageUpgradeTask>(
      store(), original_page.offline_id, temporary_file_path(),
      target_file_path(), kDummyDigest, sizeof(kContentsOfTempFile),
      callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::SUCCESS, last_status());

  auto upgraded_page =
      store_test_util()->GetPageByOfflineId(original_page.offline_id);
  ASSERT_TRUE(upgraded_page);
  EXPECT_EQ(0, upgraded_page->upgrade_attempt);
  EXPECT_EQ(target_file_path(), upgraded_page->file_path);
  EXPECT_EQ(static_cast<int64_t>(sizeof(kContentsOfTempFile)),
            upgraded_page->file_size);
  EXPECT_EQ(kDummyDigest, upgraded_page->digest);

  EXPECT_FALSE(base::PathExists(temporary_file_path()));
  EXPECT_TRUE(base::PathExists(target_file_path()));
  EXPECT_FALSE(base::PathExists(original_page.file_path));
}

TEST_F(CompleteOfflinePageUpgradeTaskTest, ItemMissing) {
  auto task = std::make_unique<CompleteOfflinePageUpgradeTask>(
      store(), 42, temporary_file_path(), target_file_path(), kDummyDigest,
      sizeof(kContentsOfTempFile), callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::ITEM_MISSING, last_status());

  EXPECT_TRUE(base::PathExists(temporary_file_path()));
  EXPECT_FALSE(base::PathExists(target_file_path()));
}

TEST_F(CompleteOfflinePageUpgradeTaskTest, TemporaryFileMissing) {
  OfflinePageItem original_page = CreateOfflinePage();

  // This ensures the temporary file won't be there.
  EXPECT_TRUE(base::DeleteFile(temporary_file_path(), false));

  auto task = std::make_unique<CompleteOfflinePageUpgradeTask>(
      store(), original_page.offline_id, temporary_file_path(),
      target_file_path(), kDummyDigest, sizeof(kContentsOfTempFile),
      callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::TEMPORARY_FILE_MISSING, last_status());

  auto upgraded_page =
      store_test_util()->GetPageByOfflineId(original_page.offline_id);
  ASSERT_TRUE(upgraded_page);
  EXPECT_EQ(original_page, *upgraded_page);

  EXPECT_FALSE(base::PathExists(temporary_file_path()));
  EXPECT_FALSE(base::PathExists(target_file_path()));
  EXPECT_TRUE(base::PathExists(original_page.file_path));
}

TEST_F(CompleteOfflinePageUpgradeTaskTest, TargetFileNameInUse) {
  OfflinePageItem original_page = CreateOfflinePage();

  // This ensures target name is taken.
  EXPECT_TRUE(base::CopyFile(temporary_file_path(), target_file_path()));

  auto task = std::make_unique<CompleteOfflinePageUpgradeTask>(
      store(), original_page.offline_id, temporary_file_path(),
      target_file_path(), kDummyDigest, sizeof(kContentsOfTempFile),
      callback());
  runner()->RunTask(std::move(task));

  EXPECT_EQ(CompleteUpgradeStatus::TARGET_FILE_NAME_IN_USE, last_status());

  auto upgraded_page =
      store_test_util()->GetPageByOfflineId(original_page.offline_id);
  ASSERT_TRUE(upgraded_page);
  EXPECT_EQ(original_page, *upgraded_page);

  EXPECT_TRUE(base::PathExists(temporary_file_path()));
  EXPECT_TRUE(base::PathExists(target_file_path()));
  EXPECT_TRUE(base::PathExists(original_page.file_path));
}

}  // namespace offline_pages
