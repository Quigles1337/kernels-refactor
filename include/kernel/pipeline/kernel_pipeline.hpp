/*
 * kernel_pipeline.hpp — Unified Pipeline: Single-Header API
 *
 * Brings together KernelState, SpectralBridge, and PalindromePrecession into
 * one coherent, end-to-end simulation pipeline.
 *
 * Quick start:
 *   #include <kernel/pipeline/kernel_pipeline.hpp>
 *   using namespace kernel::pipeline;
 *
 *   Pipeline pl = Pipeline::create(KernelMode::FULL);
 *   pl.run(16);
 *   bool ok = pl.verify_invariants();
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include <kernel/pipeline/kernel_state.hpp>
#include <kernel/quantum/palindrome_precession.hpp>
#include <kernel/pipeline/spectral_bridge.hpp>
#include <kernel/ohm/coherence_duality.hpp>

namespace kernel::pipeline {

class Pipeline {
public:
  static Pipeline create(KernelMode mode = KernelMode::FULL) {
    Pipeline pl;
    pl.mode_ = mode;
    return pl;
  }

  Pipeline &with_state(KernelState s) {
    state_ = std::move(s);
    pp_.reset();
    return *this;
  }

  Pipeline &with_logging(bool enable = true) {
    logging_ = enable;
    return *this;
  }

  Pipeline &with_mode(KernelMode mode) {
    mode_ = mode;
    return *this;
  }

  void tick() {
    std::size_t log_before = state_.renorm_log.size();
    SpectralBridge::step(state_, mode_, pp_);
    if (logging_ && state_.renorm_log.size() > log_before) {
      const auto &ev = state_.renorm_log.back();
      std::cout << "[pipeline] tick=" << ev.tick << " renorm: r " << ev.r_before
                << " -> " << ev.r_after << " R(r) " << ev.R_before
                << " -> " << ev.R_after << "\n";
    }
  }

  void run(uint64_t n) {
    for (uint64_t i = 0; i < n; ++i)
      tick();
  }

  bool verify_invariants() const { return SpectralBridge::verify(state_); }

  bool verify_spectral() const {
    return SpectralBridge::verify_spectral(state_);
  }

  const KernelState &state() const { return state_; }

  const std::vector<RenormEvent> &renorm_log() const {
    return state_.renorm_log;
  }

  kernel::ohm::CoherentChannel channel() const {
    return SpectralBridge::to_channel(state_);
  }

  KernelMode mode() const { return mode_; }

  void reset() {
    state_.reset();
    pp_.reset();
  }

private:
  KernelState state_;
  quantum::PalindromePrecession pp_;
  KernelMode mode_ = KernelMode::FULL;
  bool logging_ = false;
};

} // namespace kernel::pipeline
