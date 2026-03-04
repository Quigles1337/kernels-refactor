/*
 * arithmetic_periodicity.hpp — Arithmetic Zero-Overhead Periodicity
 *
 * Multi-scale periodic structure grounded in the palindrome quotient
 * 987654321 / 123456789 = 8 + 1/13717421.
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdint>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/core/theorems.hpp>

namespace kernel::scheduling {

struct ArithmeticPeriodicity {
  uint64_t window = 0;

  void apply_precession(QState &state) {
    double phase =
        static_cast<double>(window % static_cast<uint64_t>(PALINDROME_DENOM)) *
        (TWO_PI / PALINDROME_DENOM);
    Cx phasor{std::cos(phase), std::sin(phase)};
    state.beta *= phasor;
    ++window;
  }

  static constexpr uint8_t fast_period() { return 8; }

  static constexpr uint64_t slow_period() {
    return static_cast<uint64_t>(PALINDROME_DENOM);
  }

  static bool is_zero_overhead(double r) {
    return std::abs(palindrome_residual(r)) < RADIUS_TOL;
  }

  void reset() { window = 0; }
};

} // namespace kernel::scheduling
