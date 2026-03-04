/*
 * memory.hpp — Rotational Memory Addressing (Z/8Z)
 *
 * Memory addressing based on 8-cycle positions in Z/8Z.
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>

namespace kernel::scheduling {

class RotationalMemory {
public:
  struct MemoryBank {
    uint8_t position;
    std::vector<Cx> data;
    uint32_t access_count = 0;
    MemoryBank(uint8_t pos) : position(pos) {}
  };

  struct Address {
    uint8_t bank;
    uint32_t offset;

    Address(uint8_t b, uint32_t o) : bank(b), offset(o) {}

    static Address from_linear(uint32_t linear_addr) {
      return Address(linear_addr % 8, linear_addr / 8);
    }

    uint32_t to_linear() const { return offset * 8 + bank; }

    Address rotate(int8_t k) const {
      return Address((bank + k + 8) % 8, offset);
    }
  };

  RotationalMemory() {
    for (uint8_t i = 0; i < 8; ++i) {
      banks_.emplace_back(i);
    }
  }

  void write(const Address &addr, const Cx &value) {
    ensure_capacity(addr);
    banks_[addr.bank].data[addr.offset] = value;
    banks_[addr.bank].access_count++;
    ++total_writes_;
  }

  Cx read(const Address &addr) {
    ensure_capacity(addr);
    banks_[addr.bank].access_count++;
    ++total_reads_;
    return banks_[addr.bank].data[addr.offset];
  }

  void write_linear(uint32_t linear_addr, const Cx &value) {
    write(Address::from_linear(linear_addr), value);
  }

  Cx read_linear(uint32_t linear_addr) {
    return read(Address::from_linear(linear_addr));
  }

  void rotate_addressing(int8_t k) {
    rotation_offset_ = (rotation_offset_ + k + 8) % 8;
    ++rotation_count_;
  }

  uint8_t effective_bank(uint8_t logical_bank) const {
    return (logical_bank + rotation_offset_) % 8;
  }

  Address translate(const Address &addr) const {
    return Address(effective_bank(addr.bank), addr.offset);
  }

  const MemoryBank &get_bank(uint8_t position) const {
    return banks_[effective_bank(position)];
  }

  const std::vector<MemoryBank> &banks() const { return banks_; }

  struct Stats {
    uint64_t total_reads;
    uint64_t total_writes;
    uint32_t rotation_count;
    uint8_t rotation_offset;
    uint32_t total_capacity;
  };

  Stats get_stats() const {
    uint32_t capacity = 0;
    for (const auto &bank : banks_) {
      capacity += bank.data.size();
    }
    return Stats{total_reads_, total_writes_, rotation_count_, rotation_offset_,
                 capacity};
  }

  void report_stats() const {
    auto stats = get_stats();
    std::cout << "  Memory: " << stats.total_reads << " reads, "
              << stats.total_writes << " writes, " << stats.rotation_count
              << " rotations, " << "offset=" << (int)stats.rotation_offset
              << "/8, " << "capacity=" << stats.total_capacity << " cells\n";
  }

  bool validate_coherence() const {
    constexpr double MAX_COEFFICIENT_NORM = 100.0;
    for (const auto &bank : banks_) {
      for (const auto &coeff : bank.data) {
        double norm = std::norm(coeff);
        if (norm > MAX_COEFFICIENT_NORM) {
          return false;
        }
      }
    }
    return true;
  }

private:
  std::vector<MemoryBank> banks_;
  uint8_t rotation_offset_ = 0;
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

} // namespace kernel::scheduling
