/*
 * theorems.hpp — Theorem-Derived Functions
 *
 * Every function in this header is a direct implementation of a formally
 * stated theorem or proposition from the Pipeline of Coherence derivations.
 * Each function is defined ONCE here; all other code in the framework uses
 * these implementations rather than re-deriving them.
 *
 * Lean 4 verification status (see formal/lean/CriticalEigenvalue.lean):
 *   [✓] coherence_le_one       — C(r) ≤ 1 for all r > 0
 *   [✓] mu_pow_eight           — µ⁸ = 1
 *   [✓] rotMat_det             — det R(3π/4) = 1
 *   [✓] rotMat_orthog          — R · Rᵀ = I
 *   [✓] rotMat_pow_eight       — R(3π/4)⁸ = I
 *
 * SymPy verification: see docs/sympy_verified_formulas.tex
 *
 * Theorem references follow:
 *   docs/master_derivations.pdf  (Pipeline of Coherence — Master Derivations)
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>

namespace kernel {

// ─────────────────────────────────────────────────────────────────────────────
// Theorem 11:  Coherence function C(r) = 2r / (1 + r²)
// ─────────────────────────────────────────────────────────────────────────────
//
// Properties (all formally verified):
//   • C(r) ∈ (0, 1] for r > 0
//   • C(1) = 1  (unique global maximum)
//   • C(r) = C(1/r)  (reciprocal symmetry)
//   • C(r) → 0 as r → 0⁺ or r → ∞
//
// Lean 4:  `theorem coherence_le_one (r : ℝ) (hr : r > 0) : C r ≤ 1`

/// Compute the ℓ₁-coherence C(r) = 2r/(1+r²).
inline double coherence(double r) {
  return (2.0 * r) / (1.0 + r * r);
}

// ─────────────────────────────────────────────────────────────────────────────
// Theorem 14:  Lyapunov duality  C = sech(λ), λ = ln r
// ─────────────────────────────────────────────────────────────────────────────
//
// The Lyapunov exponent λ = ln r measures the log-distance from balance.
// At r = 1:  λ = 0, C = sech(0) = 1.
// This establishes the Ohm–Coherence Duality: C = G_eff = 1/R_eff.

/// Compute the Lyapunov exponent λ = ln r.
inline double lyapunov(double r) {
  return std::log(r);
}

/// Compute coherence from Lyapunov exponent: C = sech(λ) = 1/cosh(λ).
inline double coherence_sech(double lambda) {
  return 1.0 / std::cosh(lambda);
}

/// Compute Lyapunov exponent from coherence: λ = arccosh(1/C).
///
/// Requires 0 < C ≤ 1.  At C = 1, returns 0.
inline double lyapunov_from_coherence(double C) {
  return (C >= 1.0) ? 0.0 : std::acosh(1.0 / C);
}

// ─────────────────────────────────────────────────────────────────────────────
// Theorem 12:  Palindrome residual R(r) = (1/δ_S)(r − 1/r)
// ─────────────────────────────────────────────────────────────────────────────
//
// Properties:
//   • R(1) = 0  (balance point)
//   • R(r) > 0 for r > 1  (spiral-out)
//   • R(r) < 0 for r < 1  (spiral-in)
//   • R(r) = −R(1/r)  (antisymmetry)
//
// Corollary 13:  r = 1 ⟺ finite orbit ∧ C = 1 ∧ R = 0
//                (simultaneous break of three conditions).

/// Compute the palindrome residual R(r) = (1/δ_S)(r − 1/r).
inline double palindrome_residual(double r) {
  return (1.0 / DELTA_S) * (r - 1.0 / r);
}

// ─────────────────────────────────────────────────────────────────────────────
// Section 3:  Rotation matrix R(3π/4)
// ─────────────────────────────────────────────────────────────────────────────
//
//   R(3π/4) = [[-1/√2, -1/√2],
//              [ 1/√2, -1/√2]]
//
//   det R = 1  (proper rotation, verified in Lean: rotMat_det)
//   R · Rᵀ = I  (orthogonal, verified in Lean: rotMat_orthog)
//   R⁸ = I      (8-fold closure, verified in Lean: rotMat_pow_eight)

/// Apply the R(3π/4) rotation matrix to a 2D vector.
inline Vec2 rotate135(Vec2 v) {
  return {
    -ETA * v.x - ETA * v.y,
     ETA * v.x - ETA * v.y
  };
}

// ─────────────────────────────────────────────────────────────────────────────
// Proposition 4:  Silver conservation  δ_S · (√2 − 1) = 1
// ─────────────────────────────────────────────────────────────────────────────

/// Verify the silver conservation identity.
///
/// Returns true if |δ_S · δ_conj − 1| < tol.
/// This is checked at kernel boot as a numerical sanity test.
inline bool verify_silver_conservation(double tol = CONSERVATION_TOL) {
  return std::abs(DELTA_S * DELTA_CONJ - 1.0) < tol;
}

// ─────────────────────────────────────────────────────────────────────────────
// Ohm–Coherence bridge functions (Theorem 14)
// ─────────────────────────────────────────────────────────────────────────────
//
// These are thin wrappers expressing the duality in "circuit" language:
//   Conductance  G_eff(λ) = sech(λ) = C
//   Resistance   R_eff(λ) = cosh(λ) = 1/C

/// Effective conductance:  G_eff = sech(λ) = coherence.
inline double conductance(double lambda) {
  return 1.0 / std::cosh(lambda);
}

/// Effective resistance:  R_eff = cosh(λ) = 1/coherence.
inline double resistance(double lambda) {
  return std::cosh(lambda);
}

} // namespace kernel
