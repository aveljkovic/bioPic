#include <stdio.h>
#include <stdint.h>

#include "biopic.h"

static int expect_status(BiopicStatus actual, BiopicStatus expected, const char* label) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected status %d, got %d (%s)\n", label, (int)expected,
                (int)actual, biopic_last_error());
        return 1;
    }
    return 0;
}

int main(void) {
    BiopicEngine* engine = biopic_engine_create();
    if (engine == NULL) {
        fprintf(stderr, "biopic_engine_create failed: %s\n", biopic_last_error());
        return 1;
    }

    const uint8_t rgb[12] = {255, 128, 64, 10, 20, 30, 40, 50, 60,
                             70,  80,  90};
    BiopicFingerprint fingerprint;
    BiopicStatus status =
        biopic_hash_rgb(engine, 2, 2, rgb, sizeof(rgb), &fingerprint);
    if (expect_status(status, BIOPIC_OK, "biopic_hash_rgb")) {
        biopic_engine_destroy(engine);
        return 1;
    }

    if (fingerprint.version != BIOPIC_HASH_VERSION_1) {
        fprintf(stderr, "unexpected fingerprint version: %u\n", fingerprint.version);
        biopic_engine_destroy(engine);
        return 1;
    }

    BiopicDistance distance;
    status = biopic_compare_hashes(&fingerprint, &fingerprint, BIOPIC_DISTANCE_L1, &distance);
    if (expect_status(status, BIOPIC_OK, "biopic_compare_hashes")) {
        biopic_engine_destroy(engine);
        return 1;
    }
    if (distance.value != 0.0) {
        fprintf(stderr, "identical fingerprints must have zero L1 distance\n");
        biopic_engine_destroy(engine);
        return 1;
    }

    status = biopic_hash_rgb(NULL, 2, 2, rgb, sizeof(rgb), &fingerprint);
    if (expect_status(status, BIOPIC_ERR_INVALID_ARGUMENT, "null engine")) {
        biopic_engine_destroy(engine);
        return 1;
    }

    status = biopic_compare_hashes(NULL, &fingerprint, BIOPIC_DISTANCE_L2, &distance);
    if (expect_status(status, BIOPIC_ERR_INVALID_ARGUMENT, "null compare input")) {
        biopic_engine_destroy(engine);
        return 1;
    }

    const uint8_t garbage[4] = {0, 1, 2, 3};
    status = biopic_hash_image(engine, garbage, sizeof(garbage), &fingerprint);
    if (expect_status(status, BIOPIC_ERR_DECODE, "garbage image decode")) {
        biopic_engine_destroy(engine);
        return 1;
    }

    biopic_engine_destroy(engine);
    biopic_engine_destroy(NULL);

    printf("biopic_c_smoke: ok version=%u\n", fingerprint.version);
    return 0;
}
