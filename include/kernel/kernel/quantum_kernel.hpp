/*
 * quantum_kernel.hpp — QuantumKernel Orchestrator
 *
 * The QuantumKernel is the top-level orchestrator for the 8-cycle scheduler.
 * It manages processes, coordinates composition interactions, handles
 * decoherence interrupts, provides IPC, and enforces arithmetic periodicity.
 *
 * Boot-time invariant checks:
 *   - Prop 4: delta_S * (sqrt(2)-1) = 1 (silver conservation)
 *   - Theorem 3: eta^2 + eta^2 = 1 (critical constant)
 *   - Section 3: det R(3pi/4) = 1 (rotation matrix)
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/core/theorems.hpp>
#include <kernel/kernel/arithmetic_periodicity.hpp>
#include <kernel/kernel/memory.hpp>
#include <kernel/kernel/interrupts.hpp>
#include <kernel/kernel/ipc.hpp>
#include <kernel/kernel/process.hpp>
#include <kernel/kernel/composition.hpp>

namespace kernel::scheduling {

class QuantumKernel {
public:
  QuantumKernel() : memory_(std::make_shared<RotationalMemory>()) {
    // Prop 4: verify delta_S * (sqrt(2)-1) = 1
    double conservation = DELTA_S * DELTA_CONJ;
    if (std::abs(conservation - 1.0) > CONSERVATION_TOL)
      throw std::runtime_error("Prop 4 silver conservation violated");

    // Theorem 3: verify eta^2 + eta^2 = 1
    if (std::abs(ETA * ETA + ETA * ETA - 1.0) > CONSERVATION_TOL)
      throw std::runtime_error("Theorem 3 critical constant violated");

    // Section 3: verify det R(3pi/4) = 1
    double det = ETA * ETA + ETA * ETA;
    if (std::abs(det - 1.0) > CONSERVATION_TOL)
      throw std::runtime_error("Section 3 rotation det violated");
  }

  // ── Process composition configuration ─────────────────────────────────────
  void enable_composition(const ProcessComposition::InteractionConfig &cfg) {
    composition_enabled_ = true;
    composition_ = ProcessComposition(cfg);
  }

  void enable_composition() {
    ProcessComposition::InteractionConfig default_cfg;
    enable_composition(default_cfg);
  }

  void disable_composition() { composition_enabled_ = false; }

  // ── Interrupt handling configuration ──────────────────────────────────────
  void enable_interrupts(const interrupts::DecoherenceHandler::Config &cfg) {
    interrupts_enabled_ = true;
    interrupt_handler_ = interrupts::DecoherenceHandler(cfg);
  }

  void enable_interrupts() {
    interrupts::DecoherenceHandler::Config default_cfg;
    enable_interrupts(default_cfg);
  }

  void disable_interrupts() { interrupts_enabled_ = false; }

  // ── IPC configuration ─────────────────────────────────────────────────────
  void enable_ipc(const ipc::QuantumIPC::Config &cfg) {
    ipc_enabled_ = true;
    ipc_ = ipc::QuantumIPC(cfg);
  }

  void enable_ipc() {
    ipc::QuantumIPC::Config default_cfg;
    enable_ipc(default_cfg);
  }

  void disable_ipc() { ipc_enabled_ = false; }

  // ── Arithmetic Zero-Overhead Periodicity configuration ────────────────────
  void enable_periodicity() { periodicity_enabled_ = true; }
  void disable_periodicity() { periodicity_enabled_ = false; }

  uint32_t spawn(const std::string &name,
                 std::function<void(Process &)> task = nullptr) {
    uint32_t pid = next_pid_++;
    processes_.emplace_back(pid, name, QState{}, 0, task, false, memory_.get(),
                            &ipc_, &tick_);
    return pid;
  }

  void tick() {
    ++tick_;

    // Apply process composition before individual ticks
    if (composition_enabled_) {
      uint32_t interactions = composition_.apply_interactions(processes_);
      if (interactions > 0 && composition_.config_.log_interactions) {
        std::cout << "  tick " << tick_ << ": " << interactions
                  << " interaction(s) occurred\n";
      }
    }

    // Process ticks with decoherence interrupt handling
    for (auto &p : processes_) {
      if (interrupts_enabled_) {
        interrupt_handler_.handle_interrupt(p.pid, p.state, tick_);
      }
      p.tick();
      if (periodicity_enabled_) {
        periodicity_.apply_precession(p.state);
      }
    }
  }

  void run(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
      tick();
  }

  void report() const {
    std::cout << "\n\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90 Quantum Kernel  tick=" << tick_ << " \xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n";
    std::cout << std::fixed << std::setprecision(8);
    for (auto &p : processes_)
      p.report();

    std::cout << "\n  Prop 4:  \xce\xb4_S\xc2\xb7(\xe2\x88\x9a""2-1) = " << DELTA_S * DELTA_CONJ
              << "  (must be 1.0)\n";

    if (!processes_.empty()) {
      double r = processes_[0].state.radius();
      double C = coherence(r);
      double lam = lyapunov(r);
      double sc = coherence_sech(lam);
      std::cout << "  Thm 14:  C(r)=" << C << "  sech(\xce\xbb)=" << sc << "  match="
                << (std::abs(C - sc) < COHERENCE_TOL ? "\xe2\x9c\x93" : "\xe2\x9c\x97") << "\n";
    }

    if (composition_enabled_) {
      composition_.report_stats();
    }
    if (interrupts_enabled_) {
      interrupt_handler_.report_stats();
    }
    if (ipc_enabled_) {
      ipc_.report_stats();
    }
    memory_->report_stats();
    if (!memory_->validate_coherence()) {
      std::cout << "  \xe2\x9a\xa0 WARNING: Memory coherence validation failed\n";
    }
    std::cout << "\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n";
  }

  ProcessComposition &composition() { return composition_; }
  interrupts::DecoherenceHandler &interrupt_handler() { return interrupt_handler_; }
  ipc::QuantumIPC &ipc() { return ipc_; }
  const ipc::QuantumIPC &ipc() const { return ipc_; }
  RotationalMemory &memory() { return *memory_; }
  const RotationalMemory &memory() const { return *memory_; }

private:
  std::vector<Process> processes_;
  uint32_t next_pid_ = 1;
  uint64_t tick_ = 0;
  bool composition_enabled_ = false;
  ProcessComposition composition_;
  bool interrupts_enabled_ = false;
  interrupts::DecoherenceHandler interrupt_handler_;
  bool ipc_enabled_ = false;
  ipc::QuantumIPC ipc_;
  bool periodicity_enabled_ = false;
  ArithmeticPeriodicity periodicity_;
  std::shared_ptr<RotationalMemory> memory_;
};

} // namespace kernel::scheduling
