#include <gtest/gtest.h>

#include <string>

#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

namespace {

struct GoldenCase {
    const char* name;
    int width;
    int height;
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    const char* expected_hex;
    bool expect_zero;
};

} // namespace

// Golden vectors are locked by this table. Regenerate only when HashParameters change.
TEST(GoldenVectorsTest, KnownSyntheticImagesMatchExpectedFingerprints) {
    const GoldenCase cases[] = {
        {"uniform_black_16", 16, 16, 0, 0, 0,
         "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
         true},
        {"uniform_white_16", 16, 16, 255, 255, 255,
         "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
         true},
        {"uniform_red_32", 32, 32, 255, 0, 0,
         "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
         true},
        {"gradient_64x48", 64, 48, 0, 0, 0,
         "13424242421341e1e3e3e24141e2e3e3e14141e2e3e3e24140e1e3e3e141134242424213000000000000000000000000000000000000000000000000000000000000000000000000113a3a3a3a113ccdcdcdcd3c3ccfcfcfcf3c3dcfcfcfcf3c3ccdcdcdcd3c113a3a3a3a11000000000000000000000000000000000000000000000000000000000000000000000000",
         false},
        {"checkerboard_32", 32, 32, 0, 0, 0,
         "0860161660080fbd7272bd0f0324b2b2240316ff5555ff16096f91916f0900003c3c000000003c3c0000096f91916f0916ff5555ff160324b2b224030fbd7272bd0f086016166008080f0316090060bd24ff6f001672b255913c1672b255913c60bd24ff6f00080f03160900000916030f08006fff24bd603c9155b272163c9155b27216006fff24bd60000916030f08",
         false},
    };

    biopic::Hasher hasher;
    for (const auto& test_case : cases) {
        biopic::test_support::SyntheticImage image;
        if (std::string(test_case.name).find("gradient") != std::string::npos) {
            image = biopic::test_support::make_gradient_rgb(test_case.width, test_case.height);
        } else if (std::string(test_case.name).find("checkerboard") != std::string::npos) {
            image = biopic::test_support::make_checkerboard_rgb(test_case.width, test_case.height);
        } else {
            image = biopic::test_support::make_uniform_rgb(test_case.width, test_case.height,
                                                           test_case.r, test_case.g, test_case.b);
        }

        biopic::ImageView view(image.width, image.height, image.rgb);
        const auto fingerprint = hasher.compute(view);
        const std::string actual = biopic::test_support::fingerprint_to_hex(fingerprint.bytes);

        EXPECT_EQ(actual, test_case.expected_hex) << test_case.name;
        EXPECT_EQ(fingerprint.is_zero(), test_case.expect_zero) << test_case.name;
    }
}
