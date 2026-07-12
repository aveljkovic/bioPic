#pragma once

#include "biopic/fingerprint.hpp"

namespace biopic {

class ImageView;

class Hasher {
  public:
    explicit Hasher(std::uint32_t version = kHashAlgorithmVersion);

    [[nodiscard]] Fingerprint compute(const ImageView& image) const;

    [[nodiscard]] std::uint32_t version() const noexcept { return version_; }

  private:
    std::uint32_t version_;
};

} // namespace biopic
