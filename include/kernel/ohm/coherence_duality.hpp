/*
 * coherence_duality.hpp — Ohm-Coherence Duality Framework
 *
 * Implements the duality:  C = sech(lambda) = G_eff = 1 / R_eff
 * where lambda is the Lyapunov exponent (Theorem 14).
 *
 * Key models:
 *   CoherentChannel    : single channel characterised by its Lyapunov exponent
 *   MultiChannelSystem : N parallel channels, G_tot = Sum G_i
 *   PipelineSystem     : series stage composition, R_tot = Sum R_stage
 *   FourChannelModel   : 4-eigenvalue structure for error tolerance validation
 *   OUProcess          : Ornstein-Uhlenbeck noise on lambda
 *   QuTritDegradation  : 3-level qutrit coherence degradation patterns
 *   PhaseBattery       : phase-space analogue of an electrochemical battery
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <kernel/core/constants.hpp>

namespace kernel::ohm {

// ── Core Ohm-Coherence Duality functions (Theorem 14) ───────────────────────

inline double conductance(double lambda) { return 1.0 / std::cosh(lambda); }
inline double resistance(double lambda) { return std::cosh(lambda); }

inline double lyapunov_from_coherence(double C) {
  if (C <= 0.0 || C > 1.0 + CONSERVATION_TOL)
    throw std::domain_error("coherence must be in (0, 1]");
  double inv_C = 1.0 / C;
  if (inv_C < 1.0)
    inv_C = 1.0;
  return std::acosh(inv_C);
}

// ── CoherentChannel ─────────────────────────────────────────────────────────
struct CoherentChannel {
  double lambda;

  explicit CoherentChannel(double lam) : lambda(lam) {}

  double G_eff() const { return conductance(lambda); }
  double R_eff() const { return resistance(lambda); }
  double coherence() const { return conductance(lambda); }
};

// ── MultiChannelSystem (N parallel channels) ────────────────────────────────
struct MultiChannelSystem {
  std::vector<CoherentChannel> channels;

  explicit MultiChannelSystem(const std::vector<double> &lambdas) {
    for (double lam : lambdas)
      channels.emplace_back(lam);
  }

  MultiChannelSystem(int N, double lambda) {
    for (int i = 0; i < N; ++i)
      channels.emplace_back(lambda);
  }

  double G_total() const {
    double g = 0.0;
    for (const auto &ch : channels)
      g += ch.G_eff();
    return g;
  }

  double R_total() const { return 1.0 / G_total(); }

  int weakest_channel() const {
    int idx = 0;
    double min_g = channels[0].G_eff();
    for (int i = 1; i < static_cast<int>(channels.size()); ++i) {
      double g = channels[i].G_eff();
      if (g < min_g) {
        min_g = g;
        idx = i;
      }
    }
    return idx;
  }
};

// ── PipelineSystem (series stages) ──────────────────────────────────────────
struct PipelineSystem {
  std::vector<CoherentChannel> stages;

  explicit PipelineSystem(const std::vector<double> &lambdas) {
    for (double lam : lambdas)
      stages.emplace_back(lam);
  }

  double R_total() const {
    double r = 0.0;
    for (const auto &s : stages)
      r += s.R_eff();
    return r;
  }

  double G_total() const { return 1.0 / R_total(); }

  int bottleneck_stage() const {
    int idx = 0;
    double max_r = stages[0].R_eff();
    for (int i = 1; i < static_cast<int>(stages.size()); ++i) {
      double r = stages[i].R_eff();
      if (r > max_r) {
        max_r = r;
        idx = i;
      }
    }
    return idx;
  }
};

// ── FourChannelModel (4-eigenvalue structure) ───────────────────────────────
struct FourChannelModel {
  static constexpr int N_CHANNELS = 4;
  double lambdas[N_CHANNELS];

  FourChannelModel(double lam0, double lam1, double lam2, double lam3) {
    lambdas[0] = lam0;
    lambdas[1] = lam1;
    lambdas[2] = lam2;
    lambdas[3] = lam3;
  }

  void eigenvalues(double out[N_CHANNELS]) const {
    for (int i = 0; i < N_CHANNELS; ++i)
      out[i] = conductance(lambdas[i]);
  }

  bool validate_error_tolerance(double threshold = 0.5) const {
    int coherent = 0;
    for (int i = 0; i < N_CHANNELS; ++i)
      if (conductance(lambdas[i]) >= threshold)
        ++coherent;
    return coherent >= 3;
  }

  int weakest_channel() const {
    int idx = 0;
    double min_g = conductance(lambdas[0]);
    for (int i = 1; i < N_CHANNELS; ++i) {
      double g = conductance(lambdas[i]);
      if (g < min_g) {
        min_g = g;
        idx = i;
      }
    }
    return idx;
  }
};

// ── OUProcess (Ornstein-Uhlenbeck noise on lambda) ──────────────────────────
struct OUProcess {
  double theta;
  double mu;
  double sigma;

  OUProcess(double theta_, double mu_, double sigma_)
      : theta(theta_), mu(mu_), sigma(sigma_) {}

  std::vector<double> simulate(double lambda0, int steps, double dt,
                               uint64_t seed = 42) const {
    std::vector<double> path;
    path.reserve(steps + 1);
    path.push_back(lambda0);

    uint64_t state = seed;
    double lam = lambda0;
    for (int i = 0; i < steps; ++i) {
      double u1 = lcg_uniform(state);
      double u2 = lcg_uniform(state);
      if (u1 < 1e-15)
        u1 = 1e-15;
      double z = std::sqrt(-2.0 * std::log(u1)) * std::cos(TWO_PI * u2);
      lam += -theta * (lam - mu) * dt + sigma * std::sqrt(dt) * z;
      path.push_back(lam);
    }
    return path;
  }

  static double average_conductance(const std::vector<double> &path) {
    double sum = 0.0;
    for (double lam : path)
      sum += conductance(lam);
    return sum / static_cast<double>(path.size());
  }

private:
  static double lcg_uniform(uint64_t &s) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<double>(s >> 11) / static_cast<double>(1ULL << 53);
  }
};

// ── QuTritDegradation ───────────────────────────────────────────────────────
struct QuTritDegradation {
  double lambda_01;
  double lambda_02;
  double lambda_12;

  QuTritDegradation(double l01, double l02, double l12)
      : lambda_01(l01), lambda_02(l02), lambda_12(l12) {}

  double coherence_avg() const {
    return (conductance(lambda_01) + conductance(lambda_02) +
            conductance(lambda_12)) /
           3.0;
  }

  double coherence_min() const {
    double c01 = conductance(lambda_01);
    double c02 = conductance(lambda_02);
    double c12 = conductance(lambda_12);
    return std::min(c01, std::min(c02, c12));
  }
};

// ── metallic_oscillating_phases ─────────────────────────────────────────────
inline std::vector<double>
metallic_oscillating_phases(const std::vector<double> &phases,
                            double align_angle, double alpha = 1.0) {
  int N = static_cast<int>(phases.size());
  if (N == 0)
    return {};
  double cx = 0.0, cy = 0.0;
  for (double p : phases) {
    cx += std::cos(p);
    cy += std::sin(p);
  }
  double R = std::sqrt(cx * cx + cy * cy) / static_cast<double>(N);
  double amp = R * alpha;

  std::vector<double> out(N);
  for (int j = 0; j < N; ++j) {
    double delta = phases[j] - align_angle;
    delta =
        delta - TWO_PI * std::floor((delta + PI) / TWO_PI);
    out[j] = phases[j] - amp * delta;
  }
  return out;
}

// ── interaction_energy ──────────────────────────────────────────────────────
inline double interaction_energy(double R, int N, double g) {
  return R * R * static_cast<double>(N) * g;
}

// ── PhaseBattery ────────────────────────────────────────────────────────────
struct PhaseBattery {
  int N;
  double g;
  std::vector<double> phases;

  bool debug_mode = false;
  double alpha_c1 = 0.0;
  double alpha_c2 = 0.0;

  std::vector<double> history_E;
  std::vector<double> history_R;
  std::vector<double> history_alpha;

  PhaseBattery(int n_nodes, double gain, const std::vector<double> &init_phases)
      : N(n_nodes), g(gain), phases(init_phases) {}

  void enable_debug(bool on) { debug_mode = on; }

  void set_alpha_sensitivity(double c1, double c2) {
    alpha_c1 = c1;
    alpha_c2 = c2;
  }

  double mean_phase() const {
    double cx_val, cy_val;
    complex_sum(cx_val, cy_val);
    return std::atan2(cy_val, cx_val);
  }

  double circular_r() const {
    double cx_val, cy_val;
    complex_sum(cx_val, cy_val);
    return std::sqrt(cx_val * cx_val + cy_val * cy_val) / static_cast<double>(N);
  }

  double frustration() const {
    double psi_bar = mean_phase();
    double E = 0.0;
    for (double p : phases) {
      double d = wrap_angle(p - psi_bar);
      E += d * d;
    }
    return E / static_cast<double>(N);
  }

  double step() {
    double E_before = frustration();
    double psi_bar = mean_phase();
    for (double &p : phases)
      p -= g * wrap_angle(p - psi_bar);
    double E_after = frustration();
    return E_before - E_after;
  }

  double feedback_step(double alpha = 1.0) {
    double E_before = frustration();
    double R_before = circular_r();

    std::vector<double> delta_theta;
    if (debug_mode) {
      double psi_bar_probe = mean_phase();
      delta_theta.resize(static_cast<size_t>(N));
      for (int j = 0; j < N; ++j)
        delta_theta[static_cast<size_t>(j)] =
            wrap_angle(phases[static_cast<size_t>(j)] - psi_bar_probe);
    }

    double psi_bar = mean_phase();
    for (double &p : phases)
      p -= g * wrap_angle(p - psi_bar);

    double E_mid = frustration();
    double R_mid = circular_r();

    double delta_E = E_before - E_mid;
    double delta_R = R_mid - R_before;
    double alpha_adaptive = alpha + alpha_c1 * delta_E + alpha_c2 * delta_R;
    if (alpha_adaptive < 0.0)
      alpha_adaptive = 0.0;
    if (alpha_adaptive > 1.0)
      alpha_adaptive = 1.0;

    double g_fb = g * alpha_adaptive * R_mid;
    psi_bar = mean_phase();
    for (double &p : phases)
      p -= g_fb * wrap_angle(p - psi_bar);

    double E_after = frustration();
    double R_after = circular_r();

    if (debug_mode) {
      history_E.push_back(E_after);
      history_R.push_back(R_after);
      history_alpha.push_back(alpha_adaptive);

      std::cout << "[DEBUG] E_before=" << E_before << " E_after=" << E_after
                << " R=" << R_after << " alpha_adaptive=" << alpha_adaptive
                << "\n";
      std::cout << "[DEBUG] per-node delta_theta:";
      for (int j = 0; j < N; ++j)
        std::cout << " " << delta_theta[static_cast<size_t>(j)];
      std::cout << "\n";
    }

    return E_before - E_after;
  }

  void write_debug_csv(const std::string &filename) const {
    if (history_E.empty())
      return;
    std::ofstream f(filename);
    if (!f.is_open()) {
      std::cerr << "[PhaseBattery] write_debug_csv: cannot open '" << filename
                << "' for writing\n";
      return;
    }
    f << "step,E,R,alpha\n";
    for (size_t i = 0; i < history_E.size(); ++i)
      f << i << "," << history_E[i] << "," << history_R[i] << ","
        << history_alpha[i] << "\n";
  }

private:
  void complex_sum(double &cx_val, double &cy_val) const {
    cx_val = 0.0;
    cy_val = 0.0;
    for (double p : phases) {
      cx_val += std::cos(p);
      cy_val += std::sin(p);
    }
  }

  static double wrap_angle(double a) {
    return a - TWO_PI * std::floor((a + PI) / TWO_PI);
  }
};

} // namespace kernel::ohm
