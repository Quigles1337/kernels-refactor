/*
 * spectral_bridge.hpp — Spectral-to-Kernel Bridge and Mode-Based API
 *
 * Bridges two representations of the same physical state:
 *   Quantum representation  — KernelState (alpha, beta, r, C)
 *   Spectral representation — CoherentChannel (lambda, G_eff, R_eff)
 *
 * Kernel modes:
 *   STANDARD   — single mu-rotation per step (8-cycle)
 *   PALINDROME — mu-rotation + PalindromePrecession phase increment
 *   SPECTRAL   — mu-rotation + auto-renormalization
 *   FULL       — all of the above combined
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <kernel/core/constants.hpp>
#include <kernel/pipeline/kernel_state.hpp>
#include <kernel/quantum/palindrome_precession.hpp>
#include <kernel/ohm/coherence_duality.hpp>

namespace kernel::pipeline {

enum class KernelMode {
  STANDARD,
  PALINDROME,
  SPECTRAL,
  FULL
};

struct SpectralBridge {

  static kernel::ohm::CoherentChannel to_channel(const KernelState &ks) {
    double r = ks.radius();
    double lambda = std::log(r > 0.0 ? r : 1e-15);
    return kernel::ohm::CoherentChannel{lambda};
  }

  static KernelState from_channel(const kernel::ohm::CoherentChannel &ch,
                                  uint64_t tick = 0) {
    KernelState ks;
    ks.tick = tick;
    double r = std::exp(ch.lambda);
    double alpha_mag = ETA;
    double beta_mag = r * alpha_mag;
    double norm = std::sqrt(alpha_mag * alpha_mag + beta_mag * beta_mag);
    double a = alpha_mag / norm;
    double b = beta_mag / norm;
    static const Cx CANONICAL_BETA_PHASE{-ETA, ETA};
    ks.alpha = Cx{a, 0.0};
    ks.beta = CANONICAL_BETA_PHASE * b;
    return ks;
  }

  static void step(KernelState &ks, KernelMode mode,
                   quantum::PalindromePrecession &pp) {
    ks.step();
    if (mode == KernelMode::PALINDROME || mode == KernelMode::FULL) {
      pp.apply(ks.beta);
    }
    if (mode == KernelMode::SPECTRAL || mode == KernelMode::FULL) {
      ks.auto_renormalize();
    }
  }

  static void step(KernelState &ks, KernelMode mode) {
    quantum::PalindromePrecession pp;
    step(ks, mode, pp);
  }

  static bool verify(const KernelState &ks) { return ks.all_invariants(); }

  static bool verify_spectral(const KernelState &ks) {
    auto ch = to_channel(ks);
    return std::abs(ch.R_eff() - 1.0) < COHERENCE_TOL &&
           std::abs(ch.G_eff() - 1.0) < COHERENCE_TOL;
  }
};

} // namespace kernel::pipeline
