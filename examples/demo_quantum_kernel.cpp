/*
 * demo_quantum_kernel.cpp — Quantum Kernel Demonstration
 *
 * Demonstrates the core 8-cycle scheduler, process composition,
 * rotational memory addressing, interrupt handling, and IPC.
 */

#include <kernel/kernel.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

int main() {
  using namespace kernel;
  using namespace kernel::scheduling;

  std::cout << "Pipeline of Coherence — Quantum Kernel v2.0\n";
  std::cout << "mu = e^{i3pi/4}, eta=1/sqrt(2), 8-cycle scheduler\n\n";

  QuantumKernel kern;

  // Process at r=1: Theorem 10(i) — closed finite 8-cycle
  kern.spawn("r=1  balanced", [](Process &p) {
    if (p.cycle_pos == 0)
      std::cout << "    [balanced] completed 8-cycle  C=" << p.state.c_l1()
                << "\n";
  });

  // Process at r>1: Theorem 10(ii) — outward spiral
  kern.spawn("r>1  spiral-out", [](Process &p) {
    if (p.cycle_pos == 1 && std::abs(p.state.radius() - 1.0) < 0.01)
      p.state.beta *= 1.1;
  });

  // Process at r<1: Theorem 10(iii) — inward spiral
  kern.spawn("r<1  spiral-in", [](Process &p) {
    if (p.cycle_pos == 1 && std::abs(p.state.radius() - 1.0) < 0.01)
      p.state.beta *= 0.9;
  });

  kern.run(8);
  kern.report();

  // Corollary 13: demonstrate simultaneous break
  std::cout << "\nCorollary 13 — simultaneous break at r!=1:\n";
  for (double r : {0.5, 0.9, 1.0, 1.1, 2.0}) {
    double C = coherence(r);
    double R = palindrome_residual(r);
    double lm = lyapunov(r);
    Regime reg = classify_regime(r);
    std::cout << std::fixed << std::setprecision(6) << "  r=" << r
              << "  C=" << C << "  R=" << R
              << "  sech(lam)=" << coherence_sech(lm) << "  "
              << regime_name(reg) << "\n";
  }

  // Process Composition Demo
  std::cout << "\n\nProcess Composition Demo\n";

  QuantumKernel comp_kernel;
  ProcessComposition::InteractionConfig cfg;
  cfg.log_interactions = true;
  cfg.coupling_strength = 0.3;
  comp_kernel.enable_composition(cfg);
  comp_kernel.spawn("Quantum-A");
  comp_kernel.spawn("Quantum-B");

  comp_kernel.run(8);
  comp_kernel.report();

  // Interrupt Handling Demo
  std::cout << "\n\nInterrupt Handling Demo\n";

  QuantumKernel int_kernel;
  interrupts::DecoherenceHandler::Config int_cfg;
  int_cfg.log_interrupts = true;
  int_cfg.recovery_rate = 0.6;
  int_kernel.enable_interrupts(int_cfg);

  int_kernel.spawn("Balanced");
  int_kernel.spawn("Spiral-Out", [](Process &p) {
    if (p.cycle_pos == 1 && std::abs(p.state.radius() - 1.0) < 0.01)
      p.state.beta *= 1.2;
  });

  int_kernel.run(8);
  int_kernel.report();

  // IPC Demo
  std::cout << "\n\nIPC Demo\n";

  QuantumKernel ipc_kernel;
  ipc::QuantumIPC::Config ipc_cfg;
  ipc_cfg.log_messages = true;
  ipc_cfg.enable_coherence_check = true;
  ipc_cfg.coherence_threshold = 0.7;
  ipc_kernel.enable_ipc(ipc_cfg);

  ipc_kernel.spawn("Sender", [](Process &p) {
    if (p.cycle_pos == 0) {
      p.send_to(2, p.state.beta);
    }
  });

  ipc_kernel.spawn("Receiver", [](Process &p) {
    if (p.cycle_pos == 0) {
      auto messages = p.receive_from(1);
      if (!messages.empty()) {
        std::cout << "    [Receiver] got " << messages.size() << " msg(s)\n";
      }
    }
  });

  ipc_kernel.run(8);
  ipc_kernel.report();

  return 0;
}
