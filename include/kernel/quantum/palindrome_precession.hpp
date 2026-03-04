/*
 * palindrome_precession.hpp — Unit-Circle Invariant Palindrome Precession
 *
 * Implements uniform angular precession derived from the palindrome quotient
 * 987654321 / 123456789 = 8 + 1/13717421, ensuring that every transformation
 * has its eigenvalue signature on the unit circle (|phasor| = 1 at every step).
 *
 * Mathematical foundation:
 *   Phase increment:  DPhi = 2pi / 13717421  ~ 4.578e-7 rad/step
 *   Precession phasor at step n:  P(n) = e^{i n DPhi}
 *   Invariant:  |P(n)| = 1  for all n  (unit circle)
 *
 * Double periodicity (torus structure T^2):
 *   Fast 8-cycle:      mu = e^{i3pi/4}, period 8 steps
 *   Slow precession:   period 13717421 steps (exact 2pi return)
 *   Super-period:      lcm(8, 13717421) * alignment = 109,739,368 steps
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdint>

#include <kernel/core/constants.hpp>

namespace kernel::quantum {

// ── Palindrome arithmetic constants ──────────────────────────────────────────
// 987654321 / 123456789 = 8 + 9/123456789 = 8 + 1/13717421

static constexpr uint64_t PALINDROME_NUMERATOR    = 987654321ULL;
static constexpr uint64_t PALINDROME_DENOMINATOR  = 123456789ULL;
static constexpr uint64_t PALINDROME_INTEGER_PART = 8ULL;
static constexpr uint64_t PALINDROME_RESIDUE      = 9ULL;
static constexpr uint64_t PALINDROME_DENOM_FACTOR = 13717421ULL;

// Alias for TWO_PI used in precession context
static constexpr double PRECESSION_TWO_PI = TWO_PI;

// DPhi = 2pi / 13717421
static constexpr double PRECESSION_DELTA_PHASE =
    TWO_PI / PALINDROME_DENOM;

// ── PalindromePrecession ────────────────────────────────────────────────────
// Stateful uniform-precession operator.
//
// Maintains a step counter n and computes the unit-norm phasor P(n) =
// e^{i n DPhi} on demand.
//
// Invariant:  |P(n)| = 1 for all n  (unit circle)
struct PalindromePrecession {
  uint64_t step_count = 0;

  // Precomputed single-step phasor e^{iDPhi}
  static const std::complex<double> STEP_PHASOR;

  // Returns the precession phasor P(n) for the current step.
  std::complex<double> current_phasor() const { return phasor_at(step_count); }

  // Returns the precession phasor P(n) for an arbitrary step n.
  static std::complex<double> phasor_at(uint64_t n) {
    const double angle = static_cast<double>(n) * PRECESSION_DELTA_PHASE;
    return {std::cos(angle), std::sin(angle)};
  }

  void advance() { ++step_count; }

  // Apply one incremental precession step to a complex amplitude, then advance.
  void apply(std::complex<double> &beta) {
    beta *= STEP_PHASOR;
    advance();
  }

  void reset() { step_count = 0; }
};

// Out-of-line definition of the static step phasor e^{iDPhi}.
inline const std::complex<double> PalindromePrecession::STEP_PHASOR = {
    std::cos(PRECESSION_DELTA_PHASE), std::sin(PRECESSION_DELTA_PHASE)};

// Scaled precession rate: delta_phase(k) = 2pi / (PALINDROME_DENOM * k)
inline double precession_delta_phase(unsigned k) {
  return TWO_PI / (PALINDROME_DENOM * static_cast<double>(k));
}

} // namespace kernel::quantum
