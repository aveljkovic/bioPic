#include <benchmark/benchmark.h>
#include <string>

#include "biopic/database/fingerprint_store.hpp"
#include "biopic/index/similarity_index.hpp"
#include "biopic/types.hpp"

namespace {

biopic::Fingerprint make_fingerprint(std::uint8_t seed) {
    biopic::Fingerprint fingerprint;
    for (std::size_t index = 0; index < fingerprint.bytes.size(); ++index) {
        fingerprint.bytes[index] = static_cast<std::uint8_t>((seed + index * 17U) % 256U);
    }
    return fingerprint;
}

void populate_index(biopic::BruteForceIndex& index, std::size_t count) {
    for (std::size_t entry = 0; entry < count; ++entry) {
        biopic::FingerprintRecord record;
        record.id = std::to_string(entry);
        record.fingerprint = make_fingerprint(static_cast<std::uint8_t>(entry % 255U));
        record.label = "sample";
        benchmark::DoNotOptimize(index.add(record));
    }
}

} // namespace

static void BM_BruteForceNearest(benchmark::State& state) {
    const std::size_t record_count = static_cast<std::size_t>(state.range(0));
    biopic::BruteForceIndex index;
    populate_index(index, record_count);

    const biopic::Fingerprint query = make_fingerprint(42);
    const biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;

    for (auto _ : state) {
        const biopic::NearestMatchResult nearest = index.find_nearest(query, config);
        benchmark::DoNotOptimize(nearest.distance);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["records"] = static_cast<double>(record_count);
}

static void BM_BruteForceSimilarQuery(benchmark::State& state) {
    const std::size_t record_count = static_cast<std::size_t>(state.range(0));
    biopic::BruteForceIndex index;
    populate_index(index, record_count);

    biopic::Fingerprint query = make_fingerprint(42);
    query.bytes[0] = static_cast<std::uint8_t>(query.bytes[0] + 1U);

    biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    config.threshold = 50.0;

    for (auto _ : state) {
        const auto results = index.query(query, config);
        benchmark::DoNotOptimize(results.size());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["records"] = static_cast<double>(record_count);
}

BENCHMARK(BM_BruteForceNearest)->Arg(100)->Arg(1000)->Arg(10000);
BENCHMARK(BM_BruteForceSimilarQuery)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
