/*
 * process.hpp — Schedulable Process Unit on the 8-Cycle
 *
 * A Process is one schedulable unit in the Z/8Z quantum kernel scheduler.
 * Each process holds a QState, a cycle position in Z/8Z, and optional
 * references to shared kernel subsystems (memory, IPC).
 *
 * Per-tick evolution:
 *   1. Apply chiral non-linear rotation (Section 3 / Theorem 10)
 *   2. Advance cycle_pos by 1 in Z/8Z
 *   3. Reset interaction flag
 *   4. Execute user-defined task callback
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/core/theorems.hpp>
#include <kernel/quantum/chiral_gate.hpp>
#include <kernel/kernel/memory.hpp>
#include <kernel/kernel/ipc.hpp>

namespace kernel::scheduling {

// Forward declaration for IPC message type
using ipc::QuantumIPC;

// ── Process: one schedulable unit on the 8-cycle ─────────────────────────────
struct Process {
  uint32_t pid;
  std::string name;
  QState state;
  uint8_t cycle_pos = 0; // position in Z/8Z
  std::function<void(Process &)> task;
  bool interacted = false;              // interaction flag (per tick)
  double coherence_kick_strength = 0.0; // chiral non-linear kick (0 = linear)
  RotationalMemory *memory = nullptr;   // Shared memory reference
  QuantumIPC *ipc = nullptr;            // IPC system reference
  uint64_t *current_tick = nullptr;     // Current kernel tick

  // Constructor for explicit initialization
  Process(uint32_t pid_, std::string name_, QState state_ = QState{},
          uint8_t cycle_pos_ = 0,
          std::function<void(Process &)> task_ = nullptr,
          bool interacted_ = false, RotationalMemory *mem = nullptr,
          QuantumIPC *ipc_sys = nullptr, uint64_t *tick_ptr = nullptr)
      : pid(pid_), name(std::move(name_)), state(std::move(state_)),
        cycle_pos(cycle_pos_), task(std::move(task_)), interacted(interacted_),
        memory(mem), ipc(ipc_sys), current_tick(tick_ptr) {}

  // Memory access helpers for processes
  void mem_write(uint32_t addr, const Cx &value) {
    if (memory) {
      auto translated =
          RotationalMemory::Address::from_linear(addr).rotate(cycle_pos);
      memory->write(translated, value);
    }
  }

  Cx mem_read(uint32_t addr) {
    if (memory) {
      auto translated =
          RotationalMemory::Address::from_linear(addr).rotate(cycle_pos);
      return memory->read(translated);
    }
    return Cx{0.0, 0.0};
  }

  // ── IPC helpers for processes ─────────────────────────────────────────────

  bool send_to(uint32_t to_pid, const Cx &payload) {
    if (!ipc || !current_tick)
      return false;
    double sender_coherence = state.c_l1();
    return ipc->send_message(pid, to_pid, *current_tick, cycle_pos, payload,
                             sender_coherence);
  }

  std::vector<QuantumIPC::Message> receive_from(uint32_t from_pid) {
    if (!ipc || !current_tick)
      return {};
    double receiver_coherence = state.c_l1();
    return ipc->receive_messages(pid, from_pid, *current_tick, cycle_pos,
                                 receiver_coherence);
  }

  size_t pending_from(uint32_t from_pid) const {
    return ipc ? ipc->pending_count(from_pid, pid) : 0;
  }

  // One tick: apply chiral non-linear rotation (Section 3 / Theorem 10)
  void tick() {
    state = kernel::quantum::chiral_nonlinear(state, coherence_kick_strength);
    cycle_pos = (cycle_pos + 1) % 8; // Z/8Z arithmetic
    interacted = false;              // reset interaction flag
    if (task)
      task(*this);
  }

  // Corollary 13: all three conditions at once
  bool corollary13() const {
    double r = state.radius();
    bool orbit_closed = (std::abs(r - 1.0) < RADIUS_TOL);
    bool max_coherence = (std::abs(state.c_l1() - 1.0) < COHERENCE_TOL);
    bool palindrome_exact =
        (std::abs(state.palindrome()) < COHERENCE_TOL);
    return orbit_closed && max_coherence && palindrome_exact;
  }

  void report() const {
    double r = state.radius();
    double C = state.c_l1();
    double lam = lyapunov(r > 0 ? r : 1e-15);
    double R = state.palindrome();
    std::cout << "  PID " << pid << "  cycle=" << (int)cycle_pos
              << "  r=" << std::setw(10) << r << "  C=" << std::setw(10) << C
              << "  \u03bb=" << std::setw(10) << lam << "  sech(\u03bb)=" << std::setw(10)
              << coherence_sech(lam) << "  R(r)=" << std::setw(10) << R
              << "  Cor13=" << (corollary13() ? "\xe2\x9c\x93" : "\xe2\x9c\x97") << "  ["
              << regime_name(classify_regime(r)) << "]" << "  \"" << name << "\"\n";
  }
};

} // namespace kernel::scheduling
