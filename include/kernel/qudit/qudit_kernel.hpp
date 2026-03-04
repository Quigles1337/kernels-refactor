/*
 * qudit_kernel.hpp — Qudit Extension of Pipeline of Coherence
 *
 * Extends the quantum_kernel_v2 framework from qubits (d=2) to arbitrary
 * d-dimensional quantum systems (qudits).
 *
 * Key extensions:
 *   QuditState     : d complex coefficients |psi> = Sum_k c_k|k>, k=0..d-1
 *   QuditOps       : generalized gates — shift X_d, clock Z_d, Fourier F_d, R_d
 *   QuditEntangle  : coupling/entanglement for qudits of varying dimensions
 *   QuditMemory    : rotational memory addressing in Z/dZ
 *   QuditProcess   : schedulable unit with a d-dimensional quantum state
 *   QuditKernel    : kernel managing qudit processes on d-cycles
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>

namespace kernel::qudit {

// ── omega_d = e^{2pi*i/d}: primitive d-th root of unity ────────────────────
inline Cx omega(int d, int power = 1) {
  const double angle = TWO_PI * power / d;
  return Cx{std::cos(angle), std::sin(angle)};
}

// ── QuditState: d-dimensional pure quantum state ────────────────────────────
struct QuditState {
  int d;
  std::vector<Cx> coeffs;

  explicit QuditState(int dim) : d(dim), coeffs(dim) {
    if (dim < 2)
      throw std::invalid_argument("QuditState: dimension must be >= 2");
    double scale = 1.0 / std::sqrt(static_cast<double>(dim));
    for (int k = 0; k < dim; ++k) {
      coeffs[k] = omega(dim, k) * scale;
    }
    coeffs[0] = Cx{scale, 0.0};
  }

  QuditState(int dim, std::vector<Cx> c) : d(dim), coeffs(std::move(c)) {
    if (d < 2)
      throw std::invalid_argument("QuditState: dimension must be >= 2");
    if (static_cast<int>(coeffs.size()) != d)
      throw std::invalid_argument("QuditState: coefficient count mismatch");
    normalize();
  }

  double norm_sq() const {
    double n = 0.0;
    for (const auto &c : coeffs)
      n += std::norm(c);
    return n;
  }

  void normalize() {
    double n = std::sqrt(norm_sq());
    if (n > COHERENCE_TOL) {
      for (auto &c : coeffs)
        c /= n;
    }
  }

  double radius() const {
    if (std::abs(coeffs[0]) < COHERENCE_TOL)
      return 0.0;
    if (d == 2) {
      return std::abs(coeffs[1]) / std::abs(coeffs[0]);
    }
    double excited_sq = 0.0;
    for (int k = 1; k < d; ++k)
      excited_sq += std::norm(coeffs[k]);
    return std::sqrt(excited_sq / static_cast<double>(d - 1)) /
           std::abs(coeffs[0]);
  }

  bool balanced() const { return std::abs(radius() - 1.0) < RADIUS_TOL; }

  double c_l1() const {
    double sum = 0.0;
    for (int i = 0; i < d; ++i) {
      double ai = std::abs(coeffs[i]);
      for (int j = i + 1; j < d; ++j) {
        sum += 2.0 * ai * std::abs(coeffs[j]);
      }
    }
    return sum / static_cast<double>(d - 1);
  }

  void step() {
    for (int k = 1; k < d; ++k) {
      coeffs[k] *= omega(d, k);
    }
  }

  double coherence_fn() const {
    double r = radius();
    if (r < COHERENCE_TOL)
      return 0.0;
    return (2.0 * r) / (1.0 + r * r);
  }

  double palindrome() const {
    double r = radius();
    if (r < COHERENCE_TOL)
      return 0.0;
    return (1.0 / DELTA_S) * (r - 1.0 / r);
  }
};

// ── QuditOps: generalized quantum gate operations ───────────────────────────
class QuditOps {
public:
  using Matrix = std::vector<Cx>;

  static void apply(QuditState &state, const Matrix &gate) {
    int d = state.d;
    std::vector<Cx> out(d, Cx{0.0, 0.0});
    for (int row = 0; row < d; ++row) {
      for (int col = 0; col < d; ++col) {
        out[row] += gate[row * d + col] * state.coeffs[col];
      }
    }
    state.coeffs = std::move(out);
  }

  static Matrix shift_X(int d) {
    Matrix M(d * d, Cx{0.0, 0.0});
    for (int k = 0; k < d; ++k) {
      M[((k + 1) % d) * d + k] = Cx{1.0, 0.0};
    }
    return M;
  }

  static Matrix clock_Z(int d) {
    Matrix M(d * d, Cx{0.0, 0.0});
    for (int k = 0; k < d; ++k) {
      M[k * d + k] = omega(d, k);
    }
    return M;
  }

  static Matrix fourier_F(int d) {
    double scale = 1.0 / std::sqrt(static_cast<double>(d));
    Matrix M(d * d);
    for (int row = 0; row < d; ++row) {
      for (int col = 0; col < d; ++col) {
        M[row * d + col] = omega(d, row * col) * scale;
      }
    }
    return M;
  }

  static Matrix rotation_R(int d, double phi) {
    Matrix M(d * d, Cx{0.0, 0.0});
    for (int k = 0; k < d; ++k) {
      double angle = phi * k / d;
      M[k * d + k] = Cx{std::cos(angle), std::sin(angle)};
    }
    return M;
  }

  static double unitarity_error(const Matrix &M, int d) {
    double max_err = 0.0;
    for (int i = 0; i < d; ++i) {
      for (int j = 0; j < d; ++j) {
        Cx sum{0.0, 0.0};
        for (int k = 0; k < d; ++k) {
          sum += std::conj(M[k * d + i]) * M[k * d + j];
        }
        double expected = (i == j) ? 1.0 : 0.0;
        max_err = std::max(max_err, std::abs(sum - Cx{expected, 0.0}));
      }
    }
    return max_err;
  }
};

// ── QuditEntangle: coupling and entanglement between two qudits ─────────────
class QuditEntangle {
public:
  struct Config {
    double coupling_strength = 0.1;
    bool log_interactions = false;
    bool preserve_coherence = true;
  };

  explicit QuditEntangle(Config cfg) : config_(cfg) {}
  QuditEntangle() : config_(Config{}) {}

  bool phase_couple(QuditState &s1, QuditState &s2) {
    int coupled = std::min(s1.d, s2.d);
    double theta =
        config_.coupling_strength * PI / (2.0 * std::max(s1.d, s2.d));
    double cos_t = std::cos(theta);
    double sin_t = std::sin(theta);

    QuditState s1_init = s1;
    QuditState s2_init = s2;

    for (int k = 0; k < coupled; ++k) {
      Cx phase1 = omega(s1.d, k);
      Cx phase2 = omega(s2.d, k);
      Cx c1 = s1_init.coeffs[k];
      Cx c2 = s2_init.coeffs[k];
      s1.coeffs[k] = c1 * cos_t + c2 * phase1 * sin_t;
      s2.coeffs[k] = c2 * cos_t - c1 * std::conj(phase2) * sin_t;
    }

    s1.normalize();
    s2.normalize();

    if (config_.preserve_coherence) {
      if (s1.c_l1() > 1.0 + COHERENCE_TOL ||
          s2.c_l1() > 1.0 + COHERENCE_TOL) {
        s1 = s1_init;
        s2 = s2_init;
        ++coherence_violations_;
        return false;
      }
    }

    ++total_couplings_;
    if (config_.log_interactions) {
      std::cout << "    QuditEntangle(d1=" << s1.d << ",d2=" << s2.d
                << "): theta=" << theta << " C1=" << s1.c_l1()
                << " C2=" << s2.c_l1() << "\n";
    }
    return true;
  }

  bool controlled_shift(QuditState &control, QuditState &target) {
    if (control.d != target.d)
      return false;
    int d = control.d;

    std::vector<Cx> shifted(d, Cx{0.0, 0.0});
    for (int a = 0; a < d; ++a) {
      double weight = std::norm(control.coeffs[a]);
      if (weight < COHERENCE_TOL)
        continue;
      for (int b = 0; b < d; ++b) {
        shifted[(b + a) % d] += weight * target.coeffs[b];
      }
    }

    target.coeffs = shifted;
    target.normalize();
    ++total_couplings_;
    return true;
  }

  void report_stats() const {
    std::cout << "  QuditEntangle: " << total_couplings_ << " couplings, "
              << coherence_violations_ << " coherence violations\n";
  }

  Config config_;

private:
  uint64_t total_couplings_ = 0;
  uint64_t coherence_violations_ = 0;
};

// ── QuditMemory: Z/dZ rotational memory addressing ──────────────────────────
class QuditMemory {
public:
  struct MemoryBank {
    int position;
    std::vector<Cx> data;
    uint32_t access_count = 0;
    explicit MemoryBank(int pos) : position(pos) {}
  };

  struct Address {
    int bank;
    uint32_t offset;

    Address(int b, uint32_t o) : bank(b), offset(o) {}

    static Address from_linear(uint32_t linear_addr, int d) {
      return Address(static_cast<int>(linear_addr % static_cast<uint32_t>(d)),
                     linear_addr / static_cast<uint32_t>(d));
    }

    uint32_t to_linear(int d) const {
      return offset * static_cast<uint32_t>(d) + static_cast<uint32_t>(bank);
    }

    Address rotate(int k, int d) const {
      return Address((bank + k % d + d) % d, offset);
    }
  };

  explicit QuditMemory(int dim) : d_(dim) {
    if (dim < 2)
      throw std::invalid_argument("QuditMemory: dimension must be >= 2");
    for (int i = 0; i < dim; ++i)
      banks_.emplace_back(i);
  }

  void write(const Address &addr, const Cx &value) {
    ensure_capacity(addr);
    banks_[addr.bank].data[addr.offset] = value;
    ++banks_[addr.bank].access_count;
    ++total_writes_;
  }

  Cx read(const Address &addr) {
    ensure_capacity(addr);
    ++banks_[addr.bank].access_count;
    ++total_reads_;
    return banks_[addr.bank].data[addr.offset];
  }

  void write_linear(uint32_t linear_addr, const Cx &value) {
    write(Address::from_linear(linear_addr, d_), value);
  }

  Cx read_linear(uint32_t linear_addr) {
    return read(Address::from_linear(linear_addr, d_));
  }

  void rotate_addressing(int k) {
    rotation_offset_ = (rotation_offset_ + k % d_ + d_) % d_;
    ++rotation_count_;
  }

  int effective_bank(int logical_bank) const {
    return (logical_bank + rotation_offset_) % d_;
  }

  const std::vector<MemoryBank> &banks() const { return banks_; }
  int dimension() const { return d_; }
  uint64_t total_writes() const { return total_writes_; }
  uint64_t total_reads() const { return total_reads_; }
  uint32_t rotation_count() const { return rotation_count_; }

  bool validate_coherence() const {
    constexpr double MAX_COEFF_NORM = 100.0;
    for (const auto &bank : banks_) {
      for (const auto &coeff : bank.data) {
        if (std::norm(coeff) > MAX_COEFF_NORM)
          return false;
      }
    }
    return true;
  }

  void report_stats() const {
    uint32_t capacity = 0;
    for (const auto &bank : banks_)
      capacity += bank.data.size();
    std::cout << "  QuditMemory(d=" << d_ << "): " << total_reads_
              << " reads, " << total_writes_ << " writes, " << rotation_count_
              << " rotations, offset=" << rotation_offset_ << "/" << d_
              << ", capacity=" << capacity << " cells\n";
  }

private:
  int d_;
  std::vector<MemoryBank> banks_;
  int rotation_offset_ = 0;
  uint64_t total_reads_ = 0;
  uint64_t total_writes_ = 0;
  uint32_t rotation_count_ = 0;

  void ensure_capacity(const Address &addr) {
    auto &bank = banks_[addr.bank];
    if (addr.offset >= bank.data.size()) {
      bank.data.resize(addr.offset + 1, Cx{0.0, 0.0});
    }
  }
};

// ── QuditDecoherenceLevel ───────────────────────────────────────────────────
enum class QuditDecoherenceLevel { NONE, MINOR, MAJOR, CRITICAL };

inline QuditDecoherenceLevel measure_qudit_decoherence(double r) {
  double dev = std::abs(r - 1.0);
  if (dev <= RADIUS_TOL)
    return QuditDecoherenceLevel::NONE;
  if (dev <= DECOHERENCE_MINOR)
    return QuditDecoherenceLevel::MINOR;
  if (dev <= DECOHERENCE_MAJOR)
    return QuditDecoherenceLevel::MAJOR;
  return QuditDecoherenceLevel::CRITICAL;
}

inline const char *qudit_decoherence_name(QuditDecoherenceLevel lvl) {
  switch (lvl) {
  case QuditDecoherenceLevel::NONE: return "NONE";
  case QuditDecoherenceLevel::MINOR: return "MINOR";
  case QuditDecoherenceLevel::MAJOR: return "MAJOR";
  case QuditDecoherenceLevel::CRITICAL: return "CRITICAL";
  }
  return "";
}

// ── QuditProcess: schedulable unit on the d-cycle ───────────────────────────
struct QuditProcess {
  uint32_t pid;
  std::string name;
  QuditState state;
  int cycle_pos = 0;
  std::function<void(QuditProcess &)> task;
  bool interacted = false;
  QuditMemory *memory = nullptr;
  uint64_t *current_tick = nullptr;

  QuditProcess(uint32_t pid_, std::string name_, QuditState st,
               std::function<void(QuditProcess &)> task_ = nullptr,
               QuditMemory *mem = nullptr, uint64_t *tick_ptr = nullptr)
      : pid(pid_), name(std::move(name_)), state(std::move(st)),
        task(std::move(task_)), memory(mem), current_tick(tick_ptr) {}

  void mem_write(uint32_t addr, const Cx &value) {
    if (memory) {
      auto base = QuditMemory::Address::from_linear(addr, state.d);
      auto rotated = base.rotate(cycle_pos, state.d);
      memory->write(rotated, value);
    }
  }

  Cx mem_read(uint32_t addr) {
    if (memory) {
      auto base = QuditMemory::Address::from_linear(addr, state.d);
      auto rotated = base.rotate(cycle_pos, state.d);
      return memory->read(rotated);
    }
    return Cx{0.0, 0.0};
  }

  void tick() {
    state.step();
    cycle_pos = (cycle_pos + 1) % state.d;
    interacted = false;
    if (task)
      task(*this);
  }

  void report() const {
    double r = state.radius();
    double C = state.c_l1();
    double R = state.palindrome();
    std::cout << "  PID " << pid << "  d=" << state.d << "  cycle=" << cycle_pos
              << "  r=" << std::setw(10) << r << "  C_l1=" << std::setw(10) << C
              << "  R(r)=" << std::setw(10) << R
              << "  balanced=" << (state.balanced() ? "Y" : "N") << "  \""
              << name << "\"\n";
  }
};

// ── QuditKernel: kernel managing qudit processes ────────────────────────────
class QuditKernel {
public:
  explicit QuditKernel(int d)
      : d_(d), entangle_(), memory_(std::make_shared<QuditMemory>(d)) {
    if (d < 2)
      throw std::invalid_argument("QuditKernel: dimension must be >= 2");
    if (std::abs(DELTA_S * DELTA_CONJ - 1.0) > CONSERVATION_TOL)
      throw std::runtime_error("QuditKernel: silver conservation violated");
  }

  int dimension() const { return d_; }

  uint32_t spawn(const std::string &name,
                 std::function<void(QuditProcess &)> task = nullptr) {
    uint32_t pid = next_pid_++;
    processes_.emplace_back(pid, name, QuditState{d_}, task, memory_.get(),
                            &tick_);
    return pid;
  }

  uint32_t
  spawn_with_state(const std::string &name, QuditState st,
                   std::function<void(QuditProcess &)> task = nullptr) {
    uint32_t pid = next_pid_++;
    processes_.emplace_back(pid, name, std::move(st), task, memory_.get(),
                            &tick_);
    return pid;
  }

  void enable_entanglement(const QuditEntangle::Config &cfg) {
    entangle_enabled_ = true;
    entangle_ = QuditEntangle(cfg);
  }

  void enable_entanglement() { entangle_enabled_ = true; }

  void tick() {
    ++tick_;
    if (entangle_enabled_) {
      for (size_t i = 0; i < processes_.size(); ++i) {
        for (size_t j = i + 1; j < processes_.size(); ++j) {
          auto &pi = processes_[i];
          auto &pj = processes_[j];
          if (!pi.interacted && !pj.interacted &&
              pi.cycle_pos == pj.cycle_pos) {
            if (entangle_.phase_couple(pi.state, pj.state)) {
              pi.interacted = true;
              pj.interacted = true;
            }
          }
        }
      }
    }
    for (auto &p : processes_)
      p.tick();
  }

  void run(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
      tick();
  }

  void report() const {
    std::cout << "\nQuditKernel d=" << d_ << "  tick=" << tick_ << "\n";
    std::cout << std::fixed << std::setprecision(8);
    for (const auto &p : processes_)
      p.report();
    memory_->report_stats();
    if (!memory_->validate_coherence()) {
      std::cout << "  WARNING: d-dimensional memory coherence failed\n";
    }
    if (entangle_enabled_) {
      entangle_.report_stats();
    }
    std::cout << "  Prop 4:  delta_S*(sqrt(2)-1) = " << DELTA_S * DELTA_CONJ
              << "  (must be 1.0)\n";
  }

  uint64_t current_tick() const { return tick_; }
  QuditMemory &memory() { return *memory_; }
  const QuditMemory &memory() const { return *memory_; }
  std::vector<QuditProcess> &processes() { return processes_; }
  const std::vector<QuditProcess> &processes() const { return processes_; }

private:
  int d_;
  std::vector<QuditProcess> processes_;
  uint32_t next_pid_ = 1;
  uint64_t tick_ = 0;
  bool entangle_enabled_ = false;
  QuditEntangle entangle_;
  std::shared_ptr<QuditMemory> memory_;
};

} // namespace kernel::qudit
