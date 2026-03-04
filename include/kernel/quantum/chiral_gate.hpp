/*
 * chiral_gate.hpp — Chiral Non-Linear Gate for Quantum Pipeline
 *
 * Implements the chiral non-linear map that selectively shatters linearity
 * on the positive-imaginary domain while preserving classical reversibility
 * on the remaining domain.
 *
 * Mathematical foundation:
 *   Balance primitive mu = e^{i3pi/4}: exact 135 deg rotation R(3pi/4)
 *   Im(beta) <= 0 domain: beta' = mu*beta              (linear)
 *   Im(beta) > 0 domain:  beta' = mu*beta + k*(mu*beta)*|mu*beta| (non-linear)
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>

namespace kernel::quantum {

// ── chiral_nonlinear ────────────────────────────────────────────────────────
// Apply the balance primitive mu with selective quadratic kick.
//
//   state         — QState to evolve
//   kick_strength — magnitude of quadratic kick on Im > 0 half (0 = linear)
//
// Returns the evolved state; does NOT modify cycle_pos.
inline QState chiral_nonlinear(QState state, double kick_strength) {
  const bool positive_imag = (state.beta.imag() > 0.0);
  state.beta *= MU;
  if (positive_imag && kick_strength != 0.0) {
    state.beta += kick_strength * state.beta * std::abs(state.beta);
  }
  return state;
}

// ── run_chiral_8cycle_demo ──────────────────────────────────────────────────
// 8-cycle validation: observe linear behaviour on Im <= 0 steps and
// quadratic magnitude growth on Im > 0 steps.
inline void run_chiral_8cycle_demo(double kick_strength = 0.1) {
  std::cout << "\n\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90 Chiral Non-Linear Gate \xe2\x80\x94 8-Cycle Demo \xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n";
  std::cout << "  kick_strength = " << kick_strength << "\n";
  std::cout << std::fixed << std::setprecision(8);

  QState state;
  for (int i = 0; i < 8; ++i) {
    const bool pos_imag = (state.beta.imag() > 0.0);
    const double mag_before = std::abs(state.beta);
    state = chiral_nonlinear(state, kick_strength);
    const double mag_after = std::abs(state.beta);
    std::cout << "  step " << i << " ["
              << (pos_imag ? "Im>0 nonlin" : "Im<=0 linear") << "]"
              << "  |beta|: " << mag_before << " -> " << mag_after
              << "  D|beta|=" << (mag_after - mag_before) << "\n";
  }
  std::cout << "\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n";
}

} // namespace kernel::quantum
