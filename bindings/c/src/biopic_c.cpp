#include "biopic.h"

#include "biopic/distance.hpp"
#include "biopic/error.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include <algorithm>
#include <new>
#include <span>

struct BiopicEngine {
    biopic::ImageDecoder decoder;
    biopic::Hasher hasher;
};

BiopicEngine* biopic_engine_create() {
    try {
        return new BiopicEngine();
    } catch (...) {
        biopic::set_last_error("Failed to allocate engine");
        return nullptr;
    }
}

void biopic_engine_destroy(BiopicEngine* engine) { delete engine; }

BiopicStatus biopic_hash_rgb(BiopicEngine* engine, std::int32_t width, std::int32_t height,
                             const std::uint8_t* rgb, std::size_t rgb_size,
                             BiopicFingerprint* out_fingerprint) {
    if (engine == nullptr || out_fingerprint == nullptr || rgb == nullptr) {
        biopic::set_last_error("Invalid argument");
        return BIOPIC_ERR_INVALID_ARGUMENT;
    }
    if (width <= 0 || height <= 0) {
        biopic::set_last_error("Invalid image dimensions");
        return BIOPIC_ERR_INVALID_ARGUMENT;
    }
    const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3U;
    if (rgb_size < expected) {
        biopic::set_last_error("RGB buffer too small");
        return BIOPIC_ERR_INVALID_ARGUMENT;
    }

    try {
        const biopic::ImageView view(width, height, std::span<const std::uint8_t>(rgb, expected));
        const biopic::Fingerprint fingerprint = engine->hasher.compute(view);
        out_fingerprint->version = fingerprint.version;
        std::copy(fingerprint.bytes.begin(), fingerprint.bytes.end(), out_fingerprint->bytes);
        return BIOPIC_OK;
    } catch (const std::exception& ex) {
        biopic::set_last_error(ex.what());
        return BIOPIC_ERR_INTERNAL;
    } catch (...) {
        biopic::set_last_error("Unknown internal error");
        return BIOPIC_ERR_INTERNAL;
    }
}

BiopicStatus biopic_hash_image(BiopicEngine* engine, const std::uint8_t* data, std::size_t size,
                               BiopicFingerprint* out_fingerprint) {
    if (engine == nullptr || out_fingerprint == nullptr || data == nullptr) {
        biopic::set_last_error("Invalid argument");
        return BIOPIC_ERR_INVALID_ARGUMENT;
    }

    try {
        const auto decoded = engine->decoder.decode(std::span<const std::uint8_t>(data, size));
        if (!decoded) {
            biopic::set_last_error("Image decode failed");
            return BIOPIC_ERR_DECODE;
        }
        return biopic_hash_rgb(engine, decoded->width, decoded->height, decoded->rgb.data(),
                               decoded->rgb.size(), out_fingerprint);
    } catch (const std::exception& ex) {
        biopic::set_last_error(ex.what());
        return BIOPIC_ERR_INTERNAL;
    } catch (...) {
        biopic::set_last_error("Unknown internal error");
        return BIOPIC_ERR_INTERNAL;
    }
}

BiopicStatus biopic_compare_hashes(const BiopicFingerprint* a, const BiopicFingerprint* b,
                                   BiopicDistanceMetric metric, BiopicDistance* out) {
    if (a == nullptr || b == nullptr || out == nullptr) {
        biopic::set_last_error("Invalid argument");
        return BIOPIC_ERR_INVALID_ARGUMENT;
    }

    biopic::Fingerprint lhs{.version = a->version};
    biopic::Fingerprint rhs{.version = b->version};
    std::copy_n(a->bytes, BIOPIC_HASH_COMPONENT_COUNT, lhs.bytes.begin());
    std::copy_n(b->bytes, BIOPIC_HASH_COMPONENT_COUNT, rhs.bytes.begin());

    biopic::DistanceMetric cpp_metric = biopic::DistanceMetric::L1;
    switch (metric) {
    case BIOPIC_DISTANCE_L1:
        cpp_metric = biopic::DistanceMetric::L1;
        break;
    case BIOPIC_DISTANCE_L2:
        cpp_metric = biopic::DistanceMetric::L2;
        break;
    case BIOPIC_DISTANCE_L2_SQUARED:
        cpp_metric = biopic::DistanceMetric::L2Squared;
        break;
    }

    const auto distance = biopic::compare_fingerprints(lhs, rhs, cpp_metric);
    out->metric = metric;
    out->value = distance.value;
    return BIOPIC_OK;
}

const char* biopic_last_error() { return biopic::last_error_message().c_str(); }
