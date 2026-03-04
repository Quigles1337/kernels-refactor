/*
 * ladder_search.hpp — Ladder-based Chiral Search with optional Euler Kick
 *
 * Implements a ladder-step quantum search over n states using the Chiral
 * Non-Linear Gate balance primitive mu = e^{i3pi/4}.  An optional Euler kick
 * (e^y - 1) is applied on the positive-imaginary domain when kick_base < 1.0.
 *
 * The beta-amplitude register is stored as an Eigen::VectorXcd.
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

#include <Eigen/Dense>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/quantum/chiral_gate.hpp>

namespace kernel::quantum {

struct StepResult {
  double p_target = 0.0;
  double coherence = 0.0;
};

struct LadderChiralSearch {
  size_t target = 0;
  double kick_base = 1.0;

  StepResult ladder_step(size_t n) {
    if (n == 0)
      return {};

    std::vector<QState> states(n);

    const size_t idx = target % n;
    states[idx].beta = -states[idx].beta;

    const double kick_strength =
        (kick_base < 1.0) ? (std::exp(states[idx].beta.imag()) - 1.0) : 0.0;

    for (auto &s : states) {
      s = chiral_nonlinear(s, kick_strength);
    }

    Eigen::VectorXcd betas(static_cast<Eigen::Index>(n));
    for (size_t i = 0; i < n; ++i) {
      betas[static_cast<Eigen::Index>(i)] = states[i].beta;
    }

    const double total = betas.squaredNorm();
    const double p_target =
        (total > 0.0) ? std::norm(betas[static_cast<Eigen::Index>(idx)]) / total
                      : 0.0;

    double coh_sum = 0.0;
    for (Eigen::Index i = 0; i < static_cast<Eigen::Index>(n); ++i) {
      const double r_i = std::abs(betas[i]) / ETA;
      const double denom_i = 1.0 + r_i * r_i;
      coh_sum += (2.0 * r_i) / denom_i;
    }
    const double coh = coh_sum / static_cast<double>(n);

    return {p_target, coh};
  }
};

} // namespace kernel::quantum
