/*
 * types.hpp — Fundamental Types for the Kernel Framework
 *
 * Defines the shared type aliases, geometric primitives, and quantum state
 * structures used throughout the library.  All types live in namespace kernel
 * and depend only on constants.hpp and the C++ standard library.
 *
 * Theorem references follow:
 *   docs/master_derivations.pdf  (Pipeline of Coherence — Master Derivations)
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdint>
#include <string>

#include <kernel/core/constants.hpp>

namespace kernel {

// ─────────────────────────────────────────────────────────────────────────────
// §1  Type aliases
// ─────────────────────────────────────────────────────────────────────────────

/// Complex double — the fundamental coefficient type for quantum states.
using Cx = std::complex<double>;

// ─────────────────────────────────────────────────────────────────────────────
// §2  Balance primitive µ
// ─────────────────────────────────────────────────────────────────────────────

/// Section 2: µ = e^{i3π/4} = (−1+i)/√2  (balanced eigenvalue, second quadrant).
///
/// |µ| = 1, arg(µ) = 3π/4 = 135°.
/// µ⁸ = 1 (8-fold cyclic closure, Theorem 10).
/// R(3π/4) is the corresponding SO(2) rotation matrix with det = 1.
inline const Cx MU{-ETA, ETA};

// ─────────────────────────────────────────────────────────────────────────────
// §3  Geometric primitives
// ─────────────────────────────────────────────────────────────────────────────

/// 2D vector for rotation matrix operations.
struct Vec2 {
  double x, y;
};

// ─────────────────────────────────────────────────────────────────────────────
// §4  Quantum state (Theorem 8)
// ─────────────────────────────────────────────────────────────────────────────

/// Canonical coherent state:
///   |ψ⟩ = α|0⟩ + β|1⟩
///
/// Theorem 8:  The canonical form has α = 1/√2, β = e^{i3π/4}/√2 = µ/√2.
/// At balance: |α| = |β| = 1/√2, r = |β/α| = 1.
///
/// Theorem 9:  |α| = |β| ⟺ C_ℓ₁ = 1  (balance ↔ maximum coherence).
struct QState {
  Cx alpha;
  Cx beta;

  /// Construct the canonical coherent state (Theorem 8).
  QState()
      : alpha(ETA, 0.0),
        beta(-ETA * ETA, ETA * ETA)  // µ/√2 = (−1+i)/2
  {}

  /// Construct from arbitrary coefficients.
  QState(Cx a, Cx b) : alpha(a), beta(b) {}

  /// Radius parameter r = |β|/|α|.
  ///
  /// Theorem 10:  r = 1 → closed 8-cycle on the unit circle.
  ///              r > 1 → spiral-out.
  ///              r < 1 → spiral-in.
  double radius() const {
    double a = std::abs(alpha);
    return (a > 1e-15) ? std::abs(beta) / a : 0.0;
  }

  /// Theorem 9: ℓ₁-coherence C_ℓ₁ = 2|α||β|.
  /// At balance (|α| = |β| = 1/√2): C_ℓ₁ = 1.
  double c_l1() const { return 2.0 * std::abs(alpha) * std::abs(beta); }

  /// Theorem 12: palindrome residual R(r) on current state.
  /// R = 0 iff r = 1 (balanced).
  double palindrome() const {
    double r = radius();
    return (1.0 / DELTA_S) * (r - 1.0 / r);
  }

  /// Apply µ rotation: β ← µ·β (one step of the 8-cycle).
  /// Alias for the rotation part of tick() — some call sites use step() naming.
  void step() { beta *= MU; }

  /// Theorem 9: balanced ↔ |α| = |β| = 1/√2 ↔ C_ℓ₁ = 1.
  bool balanced() const { return std::abs(radius() - 1.0) <= RADIUS_TOL; }

  /// 8-cycle position:  k = tick mod 8, k ∈ Z/8Z.
  uint32_t cycle_pos = 0;

  /// Advance one step in the 8-cycle:  β ← µ · β, cycle_pos += 1 mod 8.
  void tick() {
    beta *= MU;
    cycle_pos = (cycle_pos + 1) % 8;
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// §5  Enumerations
// ─────────────────────────────────────────────────────────────────────────────

/// Theorem 10: Trichotomy classification based on radius r.
///
///   FINITE_ORBIT  — r = 1: closed 8-cycle, maximum coherence.
///   SPIRAL_OUT    — r > 1: expanding spiral, C(r) < 1.
///   SPIRAL_IN     — r < 1: contracting spiral, C(r) < 1.
enum class Regime {
  FINITE_ORBIT,
  SPIRAL_OUT,
  SPIRAL_IN
};

/// Classify a radius value into its trichotomy regime.
inline Regime classify_regime(double r) {
  if (std::abs(r - 1.0) <= RADIUS_TOL) return Regime::FINITE_ORBIT;
  return (r > 1.0) ? Regime::SPIRAL_OUT : Regime::SPIRAL_IN;
}

/// Decoherence severity levels for the interrupt system.
///
/// Thresholds (Section 5 of constants.hpp):
///   NONE:     |r − 1| ≤ RADIUS_TOL         (on the 8-cycle)
///   MINOR:    RADIUS_TOL < |r − 1| ≤ 0.05  (slight deviation)
///   MAJOR:    0.05 < |r − 1| ≤ 0.15        (significant deviation)
///   CRITICAL: |r − 1| > 0.15               (severe decoherence)
enum class DecoherenceLevel {
  NONE,
  MINOR,
  MAJOR,
  CRITICAL
};

/// Classify a radius value into its decoherence severity level.
inline DecoherenceLevel classify_decoherence(double r) {
  double dev = std::abs(r - 1.0);
  if (dev <= RADIUS_TOL)       return DecoherenceLevel::NONE;
  if (dev <= DECOHERENCE_MINOR) return DecoherenceLevel::MINOR;
  if (dev <= DECOHERENCE_MAJOR) return DecoherenceLevel::MAJOR;
  return DecoherenceLevel::CRITICAL;
}

/// Human-readable name for a decoherence level.
inline const char* decoherence_name(DecoherenceLevel level) {
  switch (level) {
    case DecoherenceLevel::NONE:     return "NONE";
    case DecoherenceLevel::MINOR:    return "MINOR";
    case DecoherenceLevel::MAJOR:    return "MAJOR";
    case DecoherenceLevel::CRITICAL: return "CRITICAL";
  }
  return "UNKNOWN";
}

/// Human-readable name for a trichotomy regime.
inline const char* regime_name(Regime r) {
  switch (r) {
    case Regime::FINITE_ORBIT: return "FINITE_ORBIT";
    case Regime::SPIRAL_OUT:   return "SPIRAL_OUT";
    case Regime::SPIRAL_IN:    return "SPIRAL_IN";
  }
  return "UNKNOWN";
}

} // namespace kernel
