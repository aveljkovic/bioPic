#include <gtest/gtest.h>

#include "biopic/database/fingerprint_store.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

namespace {

biopic::Fingerprint make_fingerprint(int width, int height, std::uint8_t red, std::uint8_t green,
                                     std::uint8_t blue) {
    const auto image = biopic::test_support::make_uniform_rgb(width, height, red, green, blue);
    biopic::ImageView view(image.width, image.height, image.rgb);
    biopic::Hasher hasher;
    return hasher.compute(view);
}

} // namespace

TEST(FingerprintStoreTest, StoreAndRetrieveFingerprint) {
    biopic::FingerprintStore store;
    const biopic::Fingerprint fingerprint = make_fingerprint(16, 16, 200, 100, 50);

    biopic::FingerprintRecord record;
    record.fingerprint = fingerprint;
    record.label = "known_bad";
    ASSERT_TRUE(store.add(record));
    ASSERT_EQ(store.size(), 1U);

    const auto retrieved = store.find_exact(fingerprint);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FALSE(retrieved->id.empty());
    EXPECT_EQ(retrieved->label, "known_bad");
    EXPECT_TRUE(biopic::fingerprints_equal(retrieved->fingerprint, fingerprint));
}

TEST(FingerprintStoreTest, ExactMatchDetection) {
    biopic::FingerprintStore store;
    const biopic::Fingerprint fingerprint = make_fingerprint(8, 8, 255, 0, 0);

    biopic::FingerprintRecord record;
    record.id = "abc123";
    record.fingerprint = fingerprint;
    record.label = "blocked";
    ASSERT_TRUE(store.add(record));

    const auto match = store.find_exact(fingerprint);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->id, "abc123");
    EXPECT_EQ(match->label, "blocked");
}

TEST(FingerprintStoreTest, SimilarFingerprintDetection) {
    biopic::FingerprintStore store;
    biopic::Fingerprint reference;
    reference.bytes.fill(100);
    biopic::Fingerprint query = reference;
    query.bytes[0] = 101;

    biopic::FingerprintRecord record;
    record.fingerprint = reference;
    record.label = "reference";
    ASSERT_TRUE(store.add(record));

    const auto similar = store.find_similar(query, 2.0);
    ASSERT_FALSE(similar.empty());
    EXPECT_EQ(similar.front().label, "reference");
    EXPECT_FALSE(store.find_exact(query).has_value());
}

TEST(FingerprintStoreTest, EmptyDatabaseBehavior) {
    biopic::FingerprintStore store;
    const biopic::Fingerprint fingerprint = make_fingerprint(4, 4, 10, 20, 30);

    EXPECT_FALSE(store.find_exact(fingerprint).has_value());
    EXPECT_TRUE(store.find_similar(fingerprint, 1.0).empty());

    const biopic::NearestMatchResult nearest = store.find_nearest(fingerprint);
    EXPECT_FALSE(nearest.exact_match);
    EXPECT_DOUBLE_EQ(nearest.distance, 0.0);
    EXPECT_FALSE(nearest.record.has_value());
}

TEST(FingerprintStoreTest, FindNearestReportsClosestDistance) {
    biopic::FingerprintStore store;
    biopic::Fingerprint first;
    first.bytes.fill(10);
    biopic::Fingerprint second;
    second.bytes.fill(200);
    biopic::Fingerprint query = first;
    query.bytes[1] = 11;

    biopic::FingerprintRecord first_record;
    first_record.fingerprint = first;
    first_record.label = "first";
    biopic::FingerprintRecord second_record;
    second_record.fingerprint = second;
    second_record.label = "second";
    ASSERT_TRUE(store.add(first_record));
    ASSERT_TRUE(store.add(second_record));

    const biopic::NearestMatchResult nearest = store.find_nearest(query);
    EXPECT_FALSE(nearest.exact_match);
    EXPECT_NEAR(nearest.distance, 1.0, 1e-6);
    ASSERT_TRUE(nearest.record.has_value());
    EXPECT_EQ(nearest.record->label, "first");
}

TEST(FingerprintStoreTest, RejectsRecordWithoutLabel) {
    biopic::FingerprintStore store;
    biopic::FingerprintRecord record;
    record.fingerprint = make_fingerprint(4, 4, 1, 2, 3);
    EXPECT_FALSE(store.add(record));
    EXPECT_EQ(store.size(), 0U);
}
