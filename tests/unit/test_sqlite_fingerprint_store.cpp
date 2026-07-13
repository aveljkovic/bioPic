#include <gtest/gtest.h>

#include <chrono>

#include "biopic/database/fingerprint_store.hpp"
#include "biopic/database/sqlite_fingerprint_store.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

namespace {

std::filesystem::path temp_database_path() {
    return std::filesystem::temp_directory_path() /
           ("biopic_sqlite_test_" + std::to_string(
                                        std::chrono::steady_clock::now().time_since_epoch().count()) +
            ".db");
}

biopic::Fingerprint make_fingerprint(int width, int height, std::uint8_t red, std::uint8_t green,
                                     std::uint8_t blue) {
    const auto image = biopic::test_support::make_uniform_rgb(width, height, red, green, blue);
    biopic::ImageView view(image.width, image.height, image.rgb);
    biopic::Hasher hasher;
    return hasher.compute(view);
}

} // namespace

TEST(SqliteFingerprintStoreTest, CreateInsertAndRetrieveFingerprint) {
    const std::filesystem::path database_path = temp_database_path();
    std::error_code error;

    biopic::PersistentFingerprintStore store;
    ASSERT_TRUE(store.open(database_path));

    const biopic::Fingerprint fingerprint = make_fingerprint(12, 12, 40, 80, 120);
    biopic::FingerprintRecord record;
    record.fingerprint = fingerprint;
    record.label = "known_bad";
    ASSERT_TRUE(store.add(record));
    EXPECT_EQ(store.size(), 1U);

    const auto retrieved = store.find_exact(fingerprint);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->label, "known_bad");
    EXPECT_TRUE(biopic::fingerprints_equal(retrieved->fingerprint, fingerprint));

    std::filesystem::remove(database_path, error);
}

TEST(SqliteFingerprintStoreTest, SurvivesReopenAfterClose) {
    const std::filesystem::path database_path = temp_database_path();
    std::error_code error;
    const biopic::Fingerprint fingerprint = make_fingerprint(20, 16, 15, 30, 45);

    {
        biopic::PersistentFingerprintStore store;
        ASSERT_TRUE(store.open(database_path));

        biopic::FingerprintRecord record;
        record.fingerprint = fingerprint;
        record.label = "blocked";
        ASSERT_TRUE(store.add(record));
    }

    {
        biopic::PersistentFingerprintStore store;
        ASSERT_TRUE(store.open(database_path));
        EXPECT_EQ(store.size(), 1U);

        const auto retrieved = store.find_exact(fingerprint);
        ASSERT_TRUE(retrieved.has_value());
        EXPECT_EQ(retrieved->label, "blocked");
        EXPECT_FALSE(retrieved->id.empty());
    }

    std::filesystem::remove(database_path, error);
}

TEST(SqliteFingerprintStoreTest, OpenPersistentFactoryReturnsWorkingStore) {
    const std::filesystem::path database_path = temp_database_path();
    std::error_code error;

    const auto store = biopic::open_persistent_fingerprint_store(database_path);
    ASSERT_NE(store, nullptr);

    biopic::Fingerprint fingerprint;
    fingerprint.bytes.fill(42);
    biopic::FingerprintRecord record;
    record.fingerprint = fingerprint;
    record.label = "test";
    ASSERT_TRUE(store->add(record));

    const auto retrieved = store->find_exact(fingerprint);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->label, "test");

    std::filesystem::remove(database_path, error);
}

TEST(SqliteFingerprintStoreTest, FindSimilarUsesStoredRecords) {
    const std::filesystem::path database_path = temp_database_path();
    std::error_code error;

    biopic::PersistentFingerprintStore store;
    ASSERT_TRUE(store.open(database_path));

    biopic::Fingerprint reference;
    reference.bytes.fill(90);
    biopic::Fingerprint query = reference;
    query.bytes[3] = 91;

    biopic::FingerprintRecord record;
    record.fingerprint = reference;
    record.label = "reference";
    ASSERT_TRUE(store.add(record));

    const auto similar = store.find_similar(query, 2.0);
    ASSERT_FALSE(similar.empty());
    EXPECT_EQ(similar.front().label, "reference");

    std::filesystem::remove(database_path, error);
}
