/*
 * constants.hpp — Single Source of Truth for All Mathematical Constants
 *
 * Every constant in the Kernel framework is defined here and NOWHERE ELSE.
 * All other headers and source files must include this header rather than
 * defining their own copies.
 *
 * Historical note: prior to the restructure, these constants were duplicated
 * across 9+ files with varying prefixes (KS_ETA, QUDIT_DELTA_S, MEO_ETA,
 * FS_ETA, CHIRAL_ETA, BRIDGE_ETA, PT_ETA).  This header eliminates all
 * duplication.
 *
 * Theorem references follow the numbering in:
 *   docs/master_derivations.pdf  (Pipeline of Coherence — Master Derivations)
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

namespace kernel {

// ─────────────────────────────────────────────────────────────────────────────
// §1  Fundamental constants
// ─────────────────────────────────────────────────────────────────────────────

/// π — defined here so headers remain self-contained without <cmath>.
constexpr double PI      = 3.14159265358979323846;
constexpr double TWO_PI  = 2.0 * PI;

/// Theorem 3: η = λ = 1/√2  (unique positive root of 2λ² = 1).
///
/// This is the critical constant of the Pipeline of Coherence.  It appears
/// as the modulus of both components of the canonical coherent state |ψ⟩
/// (Theorem 8) and determines the rotation angle θ = 3π/4 via the balanced
/// eigenvalue µ = e^{i3π/4}.
constexpr double ETA = 0.70710678118654752440;  // 1/√2

// ─────────────────────────────────────────────────────────────────────────────
// §2  Silver ratio (Proposition 4)
// ─────────────────────────────────────────────────────────────────────────────

/// Proposition 4: Silver ratio δ_S = 1 + √2.
///
/// Satisfies δ_S · (√2 − 1) = 1  (silver conjugate identity / energy
/// conservation).  Appears in the palindrome residual R(r) = (1/δ_S)(r − 1/r)
/// and in the conservation check verified at kernel boot.
constexpr double DELTA_S    = 2.41421356237309504880;  // δ_S = 1 + √2
constexpr double DELTA_CONJ = 0.41421356237309504880;  // √2 − 1 = 1/δ_S

// ─────────────────────────────────────────────────────────────────────────────
// §3  Palindrome precession
// ─────────────────────────────────────────────────────────────────────────────

/// Palindrome quotient:  987654321 / 123456789 = 8 + 9/123456789
///                                             = 8 + 1/13717421
///
/// The integer part 8 matches the µ 8-cycle (Theorem 10).
/// The fractional denominator 13717421 is the slow-precession period
/// providing arithmetic zero-overhead periodicity.
constexpr double PALINDROME_DENOM = 13717421.0;

/// Super-period: 8 × 13717421 = 109739368  (full µ-precession cycle).
constexpr unsigned long long SUPER_PERIOD = 8ULL * 13717421ULL;

/// ε = 1/13717421 ≈ 7.29 × 10⁻⁸  (fractional excess over integer part).
constexpr double PALINDROME_EPSILON = 1.0 / PALINDROME_DENOM;

/// Oracle rate:  8 + ε  (effective angular velocity in palindrome units).
constexpr double ORACLE_RATE = 8.0 + PALINDROME_EPSILON;

// ─────────────────────────────────────────────────────────────────────────────
// §4  Numerical tolerances
// ─────────────────────────────────────────────────────────────────────────────

/// Tolerance for coherence-related comparisons.
///
/// Used in radius r ≈ 1 detection and C(r) ≈ 1 validation.
/// At double precision, ~15 significant digits are available; 1e-9
/// provides 6 digits of headroom for accumulated rounding.
constexpr double COHERENCE_TOL    = 1e-9;

/// Tolerance for radius r = 1 detection in trichotomy classification.
constexpr double RADIUS_TOL       = 1e-9;

/// Tight tolerance for conservation law verification.
///
/// δ_S · (√2 − 1) = 1 should hold to machine epsilon (~2.2e-16);
/// 1e-12 allows 4 digits of slack.
constexpr double CONSERVATION_TOL = 1e-12;

// ─────────────────────────────────────────────────────────────────────────────
// §5  Decoherence interrupt thresholds
// ─────────────────────────────────────────────────────────────────────────────

/// Minor decoherence: RADIUS_TOL < |r − 1| ≤ 0.05.
constexpr double DECOHERENCE_MINOR = 0.05;

/// Major decoherence: 0.05 < |r − 1| ≤ 0.15.
/// Critical decoherence: |r − 1| > 0.15.
constexpr double DECOHERENCE_MAJOR = 0.15;

/// Bounds on the radius parameter during interrupt recovery.
constexpr double MIN_RECOVERY_RADIUS =  0.1;
constexpr double MAX_RECOVERY_RADIUS = 10.0;

// ─────────────────────────────────────────────────────────────────────────────
// §6  MasterEigenOracle parameters
// ─────────────────────────────────────────────────────────────────────────────

/// Threshold coefficient for θ/√n search (Theorem 14 / oracle spec).
constexpr double MEO_THRESHOLD_COEFF = 0.15;

/// Safety factor multiplier for maximum oracle steps.
constexpr double MEO_SAFETY_FACTOR = 4.0;

/// Number of eigenspace channels in the 8-fold µ decomposition.
constexpr int N_EIGENSPACE_CHANNELS = 8;

} // namespace kernel
