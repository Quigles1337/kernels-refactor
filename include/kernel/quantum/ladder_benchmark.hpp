/*
 * ladder_benchmark.hpp — Benchmark: Euler Kick vs No-Kick in LadderChiralSearch
 *
 * Measures execution time, amplification rate, and coherence C(r) of
 * LadderChiralSearch::ladder_step with and without the Euler kick.
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

#include <kernel/quantum/chiral_gate.hpp>
#include <kernel/quantum/ladder_search.hpp>

namespace kernel::quantum {

inline void benchmark_kick_vs_nokick(size_t steps = 30, size_t runs = 10,
                                     size_t n_states = 8) {
  std::cout << "\n  n=" << n_states << "  steps=" << steps << "  runs=" << runs
            << "\n";
  std::cout << std::fixed << std::setprecision(9);

  double total_time_no_kick = 0.0;
  double total_time_with_kick = 0.0;
  double total_prob_no_kick = 0.0;
  double total_prob_with_kick = 0.0;
  double total_coh_no_kick = 0.0;
  double total_coh_with_kick = 0.0;

  // Baseline: No Euler Kick
  {
    LadderChiralSearch search;
    search.target = n_states / 2;
    search.kick_base = 1.0;
    for (size_t run = 0; run < runs; ++run) {
      StepResult last{};
      auto start = std::chrono::steady_clock::now();
      for (size_t s = 0; s < steps; ++s) {
        last = search.ladder_step(n_states);
      }
      auto end = std::chrono::steady_clock::now();
      total_time_no_kick += std::chrono::duration<double>(end - start).count();
      total_prob_no_kick += last.p_target;
      total_coh_no_kick += last.coherence;
    }
  }

  // Euler Kick Active
  {
    LadderChiralSearch search;
    search.target = n_states / 2;
    search.kick_base = 0.5;
    for (size_t run = 0; run < runs; ++run) {
      StepResult last{};
      auto start = std::chrono::steady_clock::now();
      for (size_t s = 0; s < steps; ++s) {
        last = search.ladder_step(n_states);
      }
      auto end = std::chrono::steady_clock::now();
      total_time_with_kick +=
          std::chrono::duration<double>(end - start).count();
      total_prob_with_kick += last.p_target;
      total_coh_with_kick += last.coherence;
    }
  }

  const double dbl_runs = static_cast<double>(runs);
  const double avg_no_kick = total_time_no_kick / dbl_runs;
  const double avg_with_kick = total_time_with_kick / dbl_runs;
  const double avg_prob_no_kick = total_prob_no_kick / dbl_runs;
  const double avg_prob_with_kick = total_prob_with_kick / dbl_runs;
  const double avg_coh_no_kick = total_coh_no_kick / dbl_runs;
  const double avg_coh_with_kick = total_coh_with_kick / dbl_runs;

  std::cout << "  Without Euler's Kick: time=" << avg_no_kick
            << "  P(target)=" << avg_prob_no_kick
            << "  C(r)=" << avg_coh_no_kick << "\n";
  std::cout << "  With Euler's Kick:    time=" << avg_with_kick
            << "  P(target)=" << avg_prob_with_kick
            << "  C(r)=" << avg_coh_with_kick << "\n";

  if (avg_no_kick > 0.0 && avg_with_kick > 0.0) {
    std::cout << "  Speed ratio: " << avg_no_kick / avg_with_kick << "x\n";
    if (avg_prob_no_kick > 0.0)
      std::cout << "  P(target) gain: " << avg_prob_with_kick / avg_prob_no_kick << "x\n";
    if (avg_coh_no_kick > 0.0)
      std::cout << "  C(r) gain: " << avg_coh_with_kick / avg_coh_no_kick << "x\n";
  }
}

inline void benchmark_kick_vs_nokick_at_scale(
    size_t steps = 30, size_t runs = 10,
    const std::vector<size_t> &n_values = {8, 16, 32, 64, 128, 256, 512, 1024,
                                           2048, 4096}) {
  std::cout << "\nBenchmark: Euler Kick vs No-Kick -- Scale Sweep\n";
  std::cout << "  steps=" << steps << "  runs=" << runs << "\n";
  for (size_t n : n_values) {
    benchmark_kick_vs_nokick(steps, runs, n);
  }
}

} // namespace kernel::quantum
