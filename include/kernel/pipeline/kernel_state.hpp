/*
 * kernel_state.hpp — Unified State Container with Invariant Enforcement
 *
 * Wraps QState with coherent-point invariant monitoring and
 * auto-renormalization:
 *
 *   Invariant 1 — Unit-circle eigenvalue (|beta_phasor| = 1)
 *   Invariant 2 — R(r) = 0 (Theorem 12)
 *   Invariant 3 — R_eff = 1 (Ohm-Coherence Duality, Theorem 14)
 *
 * All three invariants are simultaneously satisfied at and only at r = 1
 * (Corollary 13: simultaneous break).
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/core/theorems.hpp>

namespace kernel::pipeline {

// ── RenormEvent: one auto-renormalization log entry ─────────────────────────
struct RenormEvent {
  uint64_t tick;
  double r_before;
  double r_after;
  double R_before;
  double R_after;
};

// ── KernelState: quantum state with invariant monitoring ────────────────────
struct KernelState {
  Cx alpha{ETA, 0.0};
  Cx beta{-0.5, 0.5};  // e^{i3pi/4}/sqrt(2)
  uint64_t tick = 0;
  std::vector<RenormEvent> renorm_log;

  // ── Derived quantities ────────────────────────────────────────────────────

  double radius() const {
    return std::abs(alpha) > COHERENCE_TOL ? std::abs(beta) / std::abs(alpha)
                                           : 0.0;
  }

  double coherence() const { return 2.0 * std::abs(alpha) * std::abs(beta); }

  double palindrome_residual() const {
    return kernel::palindrome_residual(radius());
  }

  double lyapunov() const {
    double r = radius();
    return std::log(r > 0.0 ? r : 1e-15);
  }

  double r_eff() const { return std::cosh(lyapunov()); }

  // ── Invariant checks ─────────────────────────────────────────────────────

  bool beta_unit_invariant() const {
    double norm_sq = std::norm(alpha) + std::norm(beta);
    return std::abs(norm_sq - 1.0) < CONSERVATION_TOL;
  }

  bool palindrome_zero() const {
    return std::abs(palindrome_residual()) < RADIUS_TOL;
  }

  bool r_eff_unity() const {
    return std::abs(r_eff() - 1.0) < COHERENCE_TOL;
  }

  bool all_invariants() const {
    return beta_unit_invariant() && palindrome_zero() && r_eff_unity();
  }

  // ── Drift detection ───────────────────────────────────────────────────────

  bool has_drift(double tol = RADIUS_TOL) const {
    return std::abs(palindrome_residual()) > tol;
  }

  // ── Normalization ─────────────────────────────────────────────────────────

  void normalize() {
    double norm_sq = std::norm(alpha) + std::norm(beta);
    if (std::abs(norm_sq - 1.0) > CONSERVATION_TOL) {
      double scale = 1.0 / std::sqrt(norm_sq);
      alpha *= scale;
      beta *= scale;
    }
  }

  // ── Auto-renormalization ──────────────────────────────────────────────────

  bool auto_renormalize(double tol = RADIUS_TOL, double rate = 0.5) {
    if (!has_drift(tol))
      return false;

    double r_before = radius();
    double R_before = palindrome_residual();

    double target_mag = std::abs(alpha);
    double current_mag = std::abs(beta);

    if (current_mag > COHERENCE_TOL && target_mag > COHERENCE_TOL) {
      double new_mag = current_mag + rate * (target_mag - current_mag);
      beta = beta * (new_mag / current_mag);
    }

    normalize();

    double r_after = radius();
    double R_after = palindrome_residual();

    renorm_log.push_back({tick, r_before, r_after, R_before, R_after});
    return true;
  }

  // ── State evolution ───────────────────────────────────────────────────────

  void step() {
    beta *= MU;
    ++tick;
  }

  void reset() {
    alpha = Cx{ETA, 0.0};
    beta = Cx{-0.5, 0.5};
    tick = 0;
    renorm_log.clear();
  }
};

} // namespace kernel::pipeline
