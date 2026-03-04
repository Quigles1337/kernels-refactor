/*
 * demo_qudit_kernel.cpp — Qudit Kernel Demonstration
 *
 * Demonstrates qudit state preparation, operations, entanglement,
 * memory, and kernel scheduling for d-dimensional quantum systems.
 */

#include <kernel/qudit/qudit_kernel.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  using namespace kernel;
  using namespace kernel::qudit;

  std::cout << "Qudit Kernel — Extension of Pipeline of Coherence\n";
  std::cout << "d-dimensional quantum process kernel\n\n";

  // 1. Qudit state preparation
  std::cout << "1. Qudit State Preparation\n";
  for (int d : {2, 3, 4, 5, 8}) {
    QuditState qs(d);
    std::cout << "  d=" << d << "  r=" << qs.radius()
              << "  C_l1=" << qs.c_l1()
              << "  balanced=" << (qs.balanced() ? "Y" : "N")
              << "  norm=" << qs.norm_sq() << "\n";
  }

  // 2. Qudit operations
  std::cout << "\n2. Qudit Operations\n";
  for (int d : {2, 3, 4}) {
    std::cout << "  d=" << d << " gates:\n";
    double err_X = QuditOps::unitarity_error(QuditOps::shift_X(d), d);
    double err_Z = QuditOps::unitarity_error(QuditOps::clock_Z(d), d);
    double err_F = QuditOps::unitarity_error(QuditOps::fourier_F(d), d);
    std::cout << "    shift X_d:   unitarity err = " << err_X
              << (err_X < COHERENCE_TOL ? " OK" : " FAIL") << "\n";
    std::cout << "    clock Z_d:   unitarity err = " << err_Z
              << (err_Z < COHERENCE_TOL ? " OK" : " FAIL") << "\n";
    std::cout << "    Fourier F_d: unitarity err = " << err_F
              << (err_F < COHERENCE_TOL ? " OK" : " FAIL") << "\n";
  }

  // 3. QuditKernel d=3 demo
  std::cout << "\n3. QuditKernel d=3\n";
  {
    QuditKernel kernel3(3);
    QuditEntangle::Config ec;
    ec.coupling_strength = 0.15;
    kernel3.enable_entanglement(ec);
    kernel3.spawn("Qutrit-A");
    kernel3.spawn("Qutrit-B");
    kernel3.run(3);
    kernel3.report();
  }

  // 4. QuditKernel d=4 demo
  std::cout << "\n4. QuditKernel d=4\n";
  {
    QuditKernel kernel4(4);
    kernel4.enable_entanglement();
    kernel4.spawn("Ququart-Writer", [](QuditProcess &p) {
      if (p.cycle_pos == 0) {
        p.mem_write(0, p.state.coeffs[0]);
      }
    });
    kernel4.spawn("Ququart-Reader", [](QuditProcess &p) {
      if (p.cycle_pos == 2) {
        Cx val = p.mem_read(0);
        std::cout << "    [Reader] read addr[0]=" << val << "\n";
      }
    });
    kernel4.run(4);
    kernel4.report();
  }

  std::cout << "\nAll qudit demonstrations complete.\n";
  return 0;
}
