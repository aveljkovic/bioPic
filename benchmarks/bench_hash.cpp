#include <benchmark/benchmark.h>

#include "biopic/distance.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

static void BM_HashResolution(benchmark::State& state) {
    const int size = static_cast<int>(state.range(0));
    const auto image = biopic::test_support::make_gradient_rgb(size, size);
    biopic::ImageView view(image.width, image.height, image.rgb);
    biopic::Hasher hasher;

    for (auto _ : state) {
        biopic::Fingerprint fingerprint = hasher.compute(view);
        benchmark::DoNotOptimize(fingerprint.bytes.data());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

static void BM_L1Comparison(benchmark::State& state) {
    biopic::Fingerprint a;
    biopic::Fingerprint b;
    for (std::size_t i = 0; i < a.bytes.size(); ++i) {
        a.bytes[i] = static_cast<std::uint8_t>(i);
        b.bytes[i] = static_cast<std::uint8_t>((i * 7) % 256);
    }

    for (auto _ : state) {
        const std::int64_t distance = biopic::l1_distance(a.bytes, b.bytes);
        benchmark::DoNotOptimize(distance);
    }
}

static void BM_L2Comparison(benchmark::State& state) {
    biopic::Fingerprint a;
    biopic::Fingerprint b;
    a.bytes.fill(12);
    b.bytes.fill(48);

    for (auto _ : state) {
        const double distance = biopic::l2_distance(a.bytes, b.bytes);
        benchmark::DoNotOptimize(distance);
    }
}

BENCHMARK(BM_HashResolution)->Arg(64)->Arg(256)->Arg(1024);
BENCHMARK(BM_L1Comparison);
BENCHMARK(BM_L2Comparison);

BENCHMARK_MAIN();
