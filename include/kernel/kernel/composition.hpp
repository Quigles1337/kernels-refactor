/*
 * composition.hpp — Process Composition: Quantum Process Interactions
 *
 * Handles quantum process interactions within Z/8Z.
 *
 * When two processes meet at the same cycle position, their quantum states
 * interact according to coherence-preserving rules that respect:
 * - Silver conservation (Prop 4)
 * - Orthogonality constraints
 * - Schedule consistency in Z/8Z
 * - Coherence function management to prevent incoherence spread
 *
 * Interaction Protocol:
 * 1. Detect when processes share same cycle_pos in Z/8Z
 * 2. Apply entanglement transformation preserving |alpha|^2+|beta|^2=1
 * 3. Exchange phase information via mu = e^{i3pi/4}
 * 4. Maintain coherence bounds C <= 1
 * 5. Verify silver conservation after interaction
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/kernel/process.hpp>

namespace kernel::scheduling {

class ProcessComposition {
public:
  static constexpr double COHERENCE_LOSS_THRESHOLD = 0.5;
  static constexpr double DAMPING_FACTOR = 0.7;

  struct InteractionConfig {
    bool enable_entanglement = true;
    bool preserve_coherence = true;
    bool log_interactions = false;
    double coupling_strength = 0.1;
  };

  ProcessComposition() : config_(InteractionConfig{}) {}
  ProcessComposition(const InteractionConfig &cfg) : config_(cfg) {}

  // Core Interaction: Apply when two processes meet in Z/8Z
  bool interact(Process &p1, Process &p2) {
    if (p1.cycle_pos != p2.cycle_pos)
      return false;
    if (p1.interacted || p2.interacted)
      return false;

    if (config_.log_interactions) {
      std::cout << "    \xe2\x8a\x97 Interaction: PID " << p1.pid << " \xe2\x86\x94 PID " << p2.pid
                << " at cycle " << (int)p1.cycle_pos << "\n";
    }

    QState s1_init = p1.state;
    QState s2_init = p2.state;
    double C1_init = s1_init.c_l1();
    double C2_init = s2_init.c_l1();

    if (config_.enable_entanglement) {
      apply_entanglement(p1.state, p2.state);
    }

    if (config_.preserve_coherence) {
      double C1_post = p1.state.c_l1();
      double C2_post = p2.state.c_l1();

      if (C1_post > 1.0 + COHERENCE_TOL ||
          C2_post > 1.0 + COHERENCE_TOL) {
        p1.state = s1_init;
        p2.state = s2_init;
        ++coherence_violations_;
        if (config_.log_interactions) {
          std::cout << "      \xe2\x9c\x97 Coherence violation, interaction rolled back\n";
        }
        return false;
      }

      if (C1_post < C1_init * COHERENCE_LOSS_THRESHOLD ||
          C2_post < C2_init * COHERENCE_LOSS_THRESHOLD) {
        apply_coherence_damping(p1.state, s1_init, DAMPING_FACTOR);
        apply_coherence_damping(p2.state, s2_init, DAMPING_FACTOR);
      }
    }

    p1.interacted = true;
    p2.interacted = true;
    ++total_interactions_;
    return true;
  }

  uint32_t apply_interactions(std::vector<Process> &processes) {
    uint32_t interaction_count = 0;
    for (size_t i = 0; i < processes.size(); ++i) {
      for (size_t j = i + 1; j < processes.size(); ++j) {
        if (interact(processes[i], processes[j])) {
          ++interaction_count;
        }
      }
    }
    return interaction_count;
  }

  void report_stats() const {
    std::cout << "  Composition stats: " << total_interactions_
              << " total interactions, " << coherence_violations_
              << " coherence violations\n";
  }

  void reset_stats() {
    total_interactions_ = 0;
    coherence_violations_ = 0;
  }

  InteractionConfig config_;

private:
  uint64_t total_interactions_ = 0;
  uint64_t coherence_violations_ = 0;

  void apply_entanglement(QState &s1, QState &s2) {
    double theta = config_.coupling_strength * PI / 8.0;
    double cos_theta = std::cos(theta);
    double sin_theta = std::sin(theta);

    Cx beta1 = s1.beta;
    Cx beta2 = s2.beta;

    s1.beta = beta1 * cos_theta + beta2 * MU * sin_theta;
    s2.beta = beta2 * cos_theta - beta1 * std::conj(MU) * sin_theta;

    renormalize(s1);
    renormalize(s2);
  }

  void renormalize(QState &s) {
    double norm_sq = std::norm(s.alpha) + std::norm(s.beta);
    if (std::abs(norm_sq - 1.0) > COHERENCE_TOL) {
      double scale = 1.0 / std::sqrt(norm_sq);
      s.alpha *= scale;
      s.beta *= scale;
    }
  }

  void apply_coherence_damping(QState &current, const QState &initial,
                               double factor) {
    current.alpha = current.alpha * factor + initial.alpha * (1.0 - factor);
    current.beta = current.beta * factor + initial.beta * (1.0 - factor);
    renormalize(current);
    ++coherence_violations_;
  }
};

} // namespace kernel::scheduling
