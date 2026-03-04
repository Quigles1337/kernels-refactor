/*
 * test_core_headers.cpp — Compile & smoke test for the restructured core.
 *
 * Verifies that constants, types, and theorem functions are correctly
 * defined and accessible from a single include path.
 */

#include <kernel/kernel.hpp>

#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>

int main() {
  using namespace kernel;

  std::cout << std::fixed << std::setprecision(15);
  int pass = 0, fail = 0;

  auto check = [&](bool cond, const char* name) {
    if (cond) { ++pass; std::cout << "  [PASS] " << name << "\n"; }
    else      { ++fail; std::cout << "  [FAIL] " << name << "\n"; }
  };

  std::cout << "=== Core Constants ===\n";
  check(std::abs(ETA - 1.0/std::sqrt(2.0)) < 1e-15,
        "ETA = 1/sqrt(2)");
  check(std::abs(DELTA_S - (1.0 + std::sqrt(2.0))) < 1e-15,
        "DELTA_S = 1 + sqrt(2)");
  check(std::abs(DELTA_CONJ - (std::sqrt(2.0) - 1.0)) < 1e-15,
        "DELTA_CONJ = sqrt(2) - 1");

  std::cout << "\n=== Proposition 4: Silver Conservation ===\n";
  check(verify_silver_conservation(),
        "delta_S * delta_conj = 1");
  check(std::abs(DELTA_S * DELTA_CONJ - 1.0) < CONSERVATION_TOL,
        "conservation within tolerance");

  std::cout << "\n=== Balance Primitive µ ===\n";
  check(std::abs(std::abs(MU) - 1.0) < 1e-15,
        "|mu| = 1");
  check(std::abs(std::arg(MU) - 3.0 * PI / 4.0) < 1e-15,
        "arg(mu) = 3pi/4");

  // µ^8 = 1
  Cx mu8 = MU;
  for (int i = 1; i < 8; ++i) mu8 *= MU;
  check(std::abs(mu8 - Cx(1.0, 0.0)) < 1e-12,
        "mu^8 = 1 (8-fold closure)");

  std::cout << "\n=== Theorem 11: Coherence C(r) ===\n";
  check(std::abs(coherence(1.0) - 1.0) < 1e-15,
        "C(1) = 1 (balance)");
  check(coherence(2.0) < 1.0,
        "C(2) < 1 (off-balance)");
  check(std::abs(coherence(0.5) - coherence(2.0)) < 1e-15,
        "C(r) = C(1/r) (reciprocal symmetry)");

  std::cout << "\n=== Theorem 14: Lyapunov Duality ===\n";
  check(std::abs(lyapunov(1.0)) < 1e-15,
        "lambda(1) = 0");
  double r_test = 2.0;
  double lam = lyapunov(r_test);
  check(std::abs(coherence(r_test) - coherence_sech(lam)) < 1e-14,
        "C(r) = sech(lambda) consistency");
  check(std::abs(conductance(lam) - coherence_sech(lam)) < 1e-15,
        "G_eff = sech(lambda) = C");
  check(std::abs(conductance(lam) * resistance(lam) - 1.0) < 1e-14,
        "G_eff * R_eff = 1");

  std::cout << "\n=== Theorem 12: Palindrome Residual ===\n";
  check(std::abs(palindrome_residual(1.0)) < 1e-15,
        "R(1) = 0");
  check(palindrome_residual(2.0) > 0.0,
        "R(r>1) > 0");
  check(palindrome_residual(0.5) < 0.0,
        "R(r<1) < 0");
  check(std::abs(palindrome_residual(2.0) + palindrome_residual(0.5)) < 1e-14,
        "R(r) = -R(1/r) (antisymmetry)");

  std::cout << "\n=== Section 3: Rotation R(3pi/4) ===\n";
  Vec2 v{1.0, 0.0};
  Vec2 v8 = v;
  for (int i = 0; i < 8; ++i) v8 = rotate135(v8);
  check(std::abs(v8.x - v.x) < 1e-12 && std::abs(v8.y - v.y) < 1e-12,
        "R^8 = I (8-fold closure on Vec2)");

  std::cout << "\n=== Types ===\n";
  QState psi;
  check(std::abs(psi.radius() - 1.0) < 1e-14,
        "canonical QState has r = 1");
  check(classify_regime(1.0) == Regime::FINITE_ORBIT,
        "r=1 -> FINITE_ORBIT");
  check(classify_regime(1.5) == Regime::SPIRAL_OUT,
        "r>1 -> SPIRAL_OUT");
  check(classify_regime(0.5) == Regime::SPIRAL_IN,
        "r<1 -> SPIRAL_IN");
  check(classify_decoherence(1.0) == DecoherenceLevel::NONE,
        "r=1 -> decoherence NONE");
  check(classify_decoherence(1.03) == DecoherenceLevel::MINOR,
        "r=1.03 -> decoherence MINOR");
  check(classify_decoherence(1.10) == DecoherenceLevel::MAJOR,
        "r=1.10 -> decoherence MAJOR");
  check(classify_decoherence(1.50) == DecoherenceLevel::CRITICAL,
        "r=1.50 -> decoherence CRITICAL");

  // QState convenience methods (FIX 1)
  check(std::abs(psi.c_l1() - 1.0) < 1e-14,
        "canonical QState C_l1 = 1 (Theorem 9)");
  check(std::abs(psi.palindrome()) < 1e-14,
        "canonical QState palindrome residual = 0");
  check(psi.balanced(),
        "canonical QState is balanced");
  QState psi2;
  psi2.step();
  check(std::abs(psi2.radius() - 1.0) < 1e-14,
        "step() preserves r = 1");

  std::cout << "\n=== Palindrome Constants ===\n";
  check(SUPER_PERIOD == 109739368ULL,
        "super period = 8 * 13717421 = 109739368");
  check(std::abs(ORACLE_RATE - (8.0 + 1.0/13717421.0)) < 1e-15,
        "oracle rate = 8 + epsilon");

  std::cout << "\n════════════════════════════════════\n";
  std::cout << "  " << pass << " passed, " << fail << " failed\n";
  std::cout << "════════════════════════════════════\n";

  return fail > 0 ? 1 : 0;
}
