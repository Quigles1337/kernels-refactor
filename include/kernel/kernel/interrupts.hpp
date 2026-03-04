/*
 * interrupts.hpp — Decoherence Interrupt System
 *
 * Interrupt handling for decoherence events: monitors phase deviation,
 * classifies severity, triggers handlers, and recovers coherence.
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/core/theorems.hpp>

namespace kernel::scheduling::interrupts {

struct DecoherenceInterrupt {
  uint64_t tick;
  uint32_t pid;
  DecoherenceLevel level;
  double r_before;
  double r_after;
  double C_before;
  double C_after;
  bool recovered;
};

class DecoherenceHandler {
public:
  struct Config {
    bool enable_interrupts = true;
    bool enable_recovery = true;
    bool log_interrupts = false;
    double recovery_rate = 0.5;
  };

  DecoherenceHandler() : config_(Config{}) {}
  DecoherenceHandler(const Config &cfg) : config_(cfg) {}

  bool handle_interrupt(uint32_t pid, QState &state, uint64_t tick) {
    double r = state.radius();
    DecoherenceLevel level = classify_decoherence(r);

    if (level == DecoherenceLevel::NONE) return false;
    if (!config_.enable_interrupts) return false;

    double r_before = r;
    double C_before = 2.0 * std::abs(state.alpha) * std::abs(state.beta);

    bool recovered = false;
    if (config_.enable_recovery) {
      recovered = apply_recovery(state, level);
    }

    double r_after = state.radius();
    double C_after = 2.0 * std::abs(state.alpha) * std::abs(state.beta);

    DecoherenceInterrupt event{tick, pid, level, r_before,
                               r_after, C_before, C_after, recovered};
    interrupt_history_.push_back(event);

    ++total_interrupts_;
    if (recovered) ++successful_recoveries_;

    if (config_.log_interrupts) {
      std::cout << "    INTERRUPT: PID " << pid
                << " decoherence=" << decoherence_name(level)
                << " r: " << r_before << " -> " << r_after << " C: " << C_before
                << " -> " << C_after << (recovered ? " OK" : " FAIL") << "\n";
    }

    return true;
  }

  void report_stats() const {
    std::cout << "  Interrupt stats: " << total_interrupts_
              << " total interrupts, " << successful_recoveries_
              << " successful recoveries";
    if (total_interrupts_ > 0) {
      double success_rate = 100.0 * successful_recoveries_ / total_interrupts_;
      std::cout << " (" << std::setprecision(1) << std::fixed << success_rate
                << "% success)";
    }
    std::cout << "\n";
  }

  void reset_stats() {
    total_interrupts_ = 0;
    successful_recoveries_ = 0;
    interrupt_history_.clear();
  }

  const std::vector<DecoherenceInterrupt> &history() const {
    return interrupt_history_;
  }

  Config config_;

private:
  uint64_t total_interrupts_ = 0;
  uint64_t successful_recoveries_ = 0;
  std::vector<DecoherenceInterrupt> interrupt_history_;

  bool apply_recovery(QState &state, DecoherenceLevel level) {
    double r = state.radius();

    if (std::abs(state.alpha) < COHERENCE_TOL) return false;

    double C_current = coherence(r);
    double coherence_defect = 1.0 - C_current;

    double base_strength = config_.recovery_rate;
    double level_multiplier = 1.0;

    switch (level) {
    case DecoherenceLevel::MINOR:    level_multiplier = 0.5; break;
    case DecoherenceLevel::MAJOR:    level_multiplier = 0.8; break;
    case DecoherenceLevel::CRITICAL: level_multiplier = 1.0; break;
    case DecoherenceLevel::NONE:     return false;
    }

    double correction_strength = base_strength * level_multiplier * coherence_defect;
    double target_r = 1.0;
    double correction_delta = (target_r - r) * correction_strength;
    double new_r = r + correction_delta;

    if (new_r < MIN_RECOVERY_RADIUS) new_r = MIN_RECOVERY_RADIUS;
    if (new_r > MAX_RECOVERY_RADIUS) new_r = MAX_RECOVERY_RADIUS;

    double current_beta_mag = std::abs(state.beta);
    double target_beta_mag = new_r * std::abs(state.alpha);

    if (current_beta_mag > COHERENCE_TOL) {
      double scale = target_beta_mag / current_beta_mag;
      state.beta *= scale;
    } else if (target_beta_mag > COHERENCE_TOL) {
      Cx phase;
      if (current_beta_mag > 0.0) {
        phase = state.beta / current_beta_mag;
      } else {
        phase = MU / std::abs(MU);
      }
      state.beta = phase * target_beta_mag;
    }

    double norm_sq = std::norm(state.alpha) + std::norm(state.beta);
    if (std::abs(norm_sq - 1.0) > COHERENCE_TOL) {
      double scale = 1.0 / std::sqrt(norm_sq);
      state.alpha *= scale;
      state.beta *= scale;
    }

    double r_new = state.radius();
    double old_deviation = std::abs(r - 1.0);
    double new_deviation = std::abs(r_new - 1.0);

    return new_deviation < old_deviation;
  }
};

} // namespace kernel::scheduling::interrupts
