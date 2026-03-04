/*
 * master_eigen_oracle.hpp — Master Eigen Oracle: Coherence-Guided Eigenspace Search
 *
 * Provides a unified oracle over the 8 eigenspaces of the balanced eigenvalue
 * mu = e^{i3pi/4}, weighted by the KernelState coherence measure
 * G_eff = sech(lambda) (Theorem 14 / Ohm-Coherence Duality).
 *
 * 8 + 1/Delta Conjecture (palindrome quotient):
 *   987654321 / 123456789 = 8 + 1/Delta, where Delta = 13,717,421.
 *   epsilon = 1/Delta ~ 7.29e-8 breaks exact 8-cycle periodicity.
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <cstdint>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/pipeline/kernel_pipeline.hpp>
#include <kernel/ohm/coherence_duality.hpp>

namespace kernel::oracle {

// Detection threshold coefficient: accumulator >= THRESHOLD_COEFF * sqrt(n)
static constexpr double MEO_THRESHOLD_COEFF = 0.15;
// Safety multiplier: run at most SAFETY_FACTOR * sqrt(n) steps
static constexpr double MEO_SAFETY_FACTOR = 4.0;
// Number of mu-eigenspace channels
static constexpr int MEO_N_CHANNELS = 8;


// ── QueryResult ─────────────────────────────────────────────────────────────
struct QueryResult {
  int best_channel = 0;
  double accumulator_peak = 0;
  uint64_t steps = 0;
  double coherence = 0;
  bool detected = false;
};

// ── CoherenceHarvest ────────────────────────────────────────────────────────
struct CoherenceHarvest {
  double harvest_score = 0.0;
  uint64_t window_steps = 0;
  int harvest_channel = 0;
  double epsilon_drift = 0.0;
};

// ── MasterEigenOracle ───────────────────────────────────────────────────────
struct MasterEigenOracle {

  MasterEigenOracle() { reset(); }

  QueryResult query(double theta_target, uint64_t n) {
    reset();
    if (n == 0)
      return {};

    const double sqrt_n = std::sqrt(static_cast<double>(n));
    const double threshold = MEO_THRESHOLD_COEFF * sqrt_n;
    const uint64_t max_steps =
        static_cast<uint64_t>(MEO_SAFETY_FACTOR * sqrt_n) + 1;
    const double delta_phi = TWO_PI / sqrt_n;

    std::array<Cx, MEO_N_CHANNELS> mu_orbit = build_mu_orbit();
    const Cx target_phasor{std::cos(theta_target), std::sin(theta_target)};

    uint64_t k = 0;
    for (; k < max_steps; ++k) {
      const double angle_k = static_cast<double>(k) * delta_phi;
      const Cx slow_phasor{std::cos(angle_k), std::sin(angle_k)};
      const double g_eff = pipeline_channel_g_eff();

      for (int j = 0; j < MEO_N_CHANNELS; ++j) {
        const Cx probe = slow_phasor * mu_orbit[j];
        const double contrib =
            g_eff * (probe * std::conj(target_phasor)).real();
        accumulators_[j] += contrib;
      }

      pipeline_.tick();

      const double peak = max_abs_accumulator();
      if (peak >= threshold) {
        ++k;
        break;
      }
    }

    QueryResult result;
    result.steps = k;
    result.best_channel = best_channel_index();
    result.accumulator_peak = max_abs_accumulator();
    result.coherence = pipeline_channel_g_eff();
    result.detected = (result.accumulator_peak >= threshold);
    return result;
  }

  const std::array<double, MEO_N_CHANNELS> &accumulators() const {
    return accumulators_;
  }

  double coherence() const { return pipeline_channel_g_eff(); }
  double radius() const { return pipeline_.state().radius(); }

  bool validate_four_channel(double threshold = 0.5) const {
    using kernel::ohm::FourChannelModel;
    double r = pipeline_.state().radius();
    double lam = std::abs(std::log(r > 0.0 ? r : 1e-15));
    FourChannelModel fcm{lam, lam, lam, lam};
    return fcm.validate_error_tolerance(threshold);
  }

  static constexpr double symmetry_breaking_factor() { return PALINDROME_EPSILON; }

  CoherenceHarvest harvest_coherence(double theta_target, uint64_t window) {
    if (window == 0)
      return {};

    const Cx target_phasor{std::cos(theta_target), std::sin(theta_target)};
    const auto mu_orbit = build_mu_orbit();
    const double delta_phi =
        TWO_PI / std::sqrt(static_cast<double>(window));

    double total_g_eff = 0.0;
    std::array<double, MEO_N_CHANNELS> channel_sum{};
    channel_sum.fill(0.0);

    for (uint64_t k = 0; k < window; ++k) {
      const double angle_k = static_cast<double>(k) * delta_phi;
      const Cx slow_phasor{std::cos(angle_k), std::sin(angle_k)};
      const double g_eff = pipeline_channel_g_eff();
      total_g_eff += g_eff;

      for (int j = 0; j < MEO_N_CHANNELS; ++j) {
        const Cx probe = slow_phasor * mu_orbit[j];
        channel_sum[j] += g_eff * (probe * std::conj(target_phasor)).real();
      }

      pipeline_.tick();
    }

    int best = 0;
    double peak = std::abs(channel_sum[0]);
    for (int j = 1; j < MEO_N_CHANNELS; ++j) {
      double v = std::abs(channel_sum[j]);
      if (v > peak) {
        peak = v;
        best = j;
      }
    }

    CoherenceHarvest h;
    h.harvest_score = total_g_eff / static_cast<double>(window);
    h.window_steps = window;
    h.harvest_channel = best;
    h.epsilon_drift = static_cast<double>(window) * PALINDROME_EPSILON;
    return h;
  }

  void reset() {
    accumulators_.fill(0.0);
    pipeline_ =
        kernel::pipeline::Pipeline::create(kernel::pipeline::KernelMode::FULL);
  }

  static std::array<Cx, MEO_N_CHANNELS> build_mu_orbit() {
    std::array<Cx, MEO_N_CHANNELS> orbit;
    Cx power{1.0, 0.0};
    for (int j = 0; j < MEO_N_CHANNELS; ++j) {
      orbit[j] = power;
      power *= MU;
    }
    return orbit;
  }

  static double oracle_contrib(double probe_angle, double theta_target,
                               double g_eff) {
    return g_eff * std::cos(probe_angle - theta_target);
  }

private:
  kernel::pipeline::Pipeline pipeline_{
      kernel::pipeline::Pipeline::create(kernel::pipeline::KernelMode::FULL)};
  std::array<double, MEO_N_CHANNELS> accumulators_{};

  double pipeline_channel_g_eff() const { return pipeline_.channel().G_eff(); }

  int best_channel_index() const {
    int best = 0;
    double peak = std::abs(accumulators_[0]);
    for (int j = 1; j < MEO_N_CHANNELS; ++j) {
      double v = std::abs(accumulators_[j]);
      if (v > peak) {
        peak = v;
        best = j;
      }
    }
    return best;
  }

  double max_abs_accumulator() const {
    double peak = 0.0;
    for (int j = 0; j < MEO_N_CHANNELS; ++j) {
      double v = std::abs(accumulators_[j]);
      if (v > peak)
        peak = v;
    }
    return peak;
  }
};

} // namespace kernel::oracle
