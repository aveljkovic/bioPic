#pragma once

#include "biopic/types.hpp"

namespace biopic {

struct ScanResult;

struct PolicyConfig {
    double classifier_block_threshold = 0.97;
    double classifier_review_threshold = 0.70;
    ModerationDecision exact_match_decision = ModerationDecision::Block;
    ModerationDecision similar_match_decision = ModerationDecision::Review;
    ModerationDecision default_decision = ModerationDecision::Allow;
};

constexpr PolicyConfig kDefaultPolicyConfig{};

struct PolicyEvaluation {
    ModerationDecision decision = ModerationDecision::Allow;
    ModerationReason reason = ModerationReason::None;
};

class ModerationPolicy {
  public:
    explicit ModerationPolicy(PolicyConfig config = kDefaultPolicyConfig);

    [[nodiscard]] PolicyEvaluation evaluate(const ScanResult& result) const;

    [[nodiscard]] const PolicyConfig& config() const noexcept { return config_; }

  private:
    PolicyConfig config_;
};

[[nodiscard]] PolicyEvaluation evaluate_policy(const ScanResult& result,
                                               const PolicyConfig& config = kDefaultPolicyConfig);

[[nodiscard]] ModerationDecision scan_decision(const ScanResult& result);

[[nodiscard]] ModerationDecision scan_decision(const ScanResult& result,
                                               const PolicyConfig& config);

[[nodiscard]] const char* moderation_reason_label(ModerationReason reason);

} // namespace biopic
