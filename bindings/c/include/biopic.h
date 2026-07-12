#ifndef BIOPIC_H
#define BIOPIC_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(BIOPIC_C_EXPORTS)
#define BIOPIC_API __declspec(dllexport)
#else
#define BIOPIC_API __declspec(dllimport)
#endif
#else
#define BIOPIC_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BIOPIC_HASH_COMPONENT_COUNT 144U
#define BIOPIC_HASH_VERSION_1 1U

typedef struct BiopicEngine BiopicEngine;

typedef struct BiopicFingerprint {
    uint32_t version;
    uint8_t bytes[BIOPIC_HASH_COMPONENT_COUNT];
} BiopicFingerprint;

typedef enum BiopicDistanceMetric {
    BIOPIC_DISTANCE_L1 = 0,
    BIOPIC_DISTANCE_L2 = 1,
    BIOPIC_DISTANCE_L2_SQUARED = 2
} BiopicDistanceMetric;

typedef struct BiopicDistance {
    BiopicDistanceMetric metric;
    double value;
} BiopicDistance;

typedef enum BiopicStatus {
    BIOPIC_OK = 0,
    BIOPIC_ERR_INVALID_ARGUMENT = 1,
    BIOPIC_ERR_DECODE = 2,
    BIOPIC_ERR_INTERNAL = 3,
    BIOPIC_ERR_UNSUPPORTED = 4
} BiopicStatus;

BIOPIC_API BiopicEngine* biopic_engine_create(void);
BIOPIC_API void biopic_engine_destroy(BiopicEngine* engine);

BIOPIC_API BiopicStatus biopic_hash_image(BiopicEngine* engine, const uint8_t* data, size_t size,
                                           BiopicFingerprint* out_fingerprint);

BIOPIC_API BiopicStatus biopic_hash_rgb(BiopicEngine* engine, int32_t width, int32_t height,
                                        const uint8_t* rgb, size_t rgb_size,
                                        BiopicFingerprint* out_fingerprint);

BIOPIC_API BiopicStatus biopic_compare_hashes(const BiopicFingerprint* a,
                                              const BiopicFingerprint* b,
                                              BiopicDistanceMetric metric, BiopicDistance* out);

BIOPIC_API const char* biopic_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* BIOPIC_H */
