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
         "00020d130200041660381b010a332c95fe0c0d398ca6b365166181ffffb2011c5b48231000050d070d090a3a1d376e11095346387f5f088d5a71e33d126ddfa8ff8c1443243ecc72040612111001061d32427f3a0e404276ff78055677afaa1a1f63dec3ff3c0c0a4bb69e13000706010c0b0e3d47426834073365b0d8420a3b827cff6209888f927f3c011a3a473106",
         false},
        {"uniform_red_32", 32, 32, 255, 0, 0,
         "000209112c07031d4340bb3e04578179d65917585f8bff842ca54affff6c144d21516f6c05080a140c0c112938815228132e85c3ca590d2b74999e040344ceacdaad00165d51211101091110151008275085452016408aa27c2a1a6371adff762074a1b1ff2e03142b361c07020f141a1c0309233a488f1a0f254e725e3c283876f2b51f1a288d6fff7414182b26fb1c",
         false},
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
