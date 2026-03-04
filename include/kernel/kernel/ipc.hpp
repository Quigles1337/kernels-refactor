/*
 * ipc.hpp — Inter-Process Communication with Coherence Preservation
 *
 * Message passing model with Z/8Z-aware channels and coherence gating.
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <iostream>
#include <vector>

#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>

namespace kernel::scheduling::ipc {

class QuantumIPC {
public:
  struct Message {
    uint32_t sender_pid;
    uint32_t receiver_pid;
    uint64_t timestamp;
    uint8_t sender_cycle_pos;
    Cx payload;
    double sender_coherence;

    Message(uint32_t from, uint32_t to, uint64_t tick, uint8_t pos,
            const Cx &data, double coh)
        : sender_pid(from), receiver_pid(to), timestamp(tick),
          sender_cycle_pos(pos), payload(data), sender_coherence(coh) {}
  };

  struct Channel {
    uint32_t sender_pid;
    uint32_t receiver_pid;
    std::vector<Message> queue;
    uint64_t messages_sent = 0;
    uint64_t messages_delivered = 0;

    Channel(uint32_t from, uint32_t to) : sender_pid(from), receiver_pid(to) {}
  };

  struct Config {
    bool enable_coherence_check = true;
    bool enable_cycle_alignment = true;
    bool log_messages = false;
    double coherence_threshold = 0.5;
    uint32_t max_queue_size = 100;
  };

  QuantumIPC() : config_(Config{}) {}
  QuantumIPC(const Config &cfg) : config_(cfg) {}

  bool send_message(uint32_t from_pid, uint32_t to_pid, uint64_t tick,
                    uint8_t sender_pos, const Cx &data,
                    double sender_coherence) {
    if (config_.enable_coherence_check) {
      if (sender_coherence < config_.coherence_threshold) {
        if (config_.log_messages) {
          std::cout << "    X IPC: PID " << from_pid << " -> PID " << to_pid
                    << " BLOCKED (sender coherence too low: "
                    << sender_coherence << ")\n";
        }
        ++blocked_sends_;
        return false;
      }
    }

    constexpr double MAX_PAYLOAD_NORM = 100.0;
    if (std::norm(data) > MAX_PAYLOAD_NORM) {
      ++blocked_sends_;
      return false;
    }

    auto &channel = get_or_create_channel(from_pid, to_pid);

    if (channel.queue.size() >= config_.max_queue_size) {
      ++blocked_sends_;
      return false;
    }

    channel.queue.emplace_back(from_pid, to_pid, tick, sender_pos, data,
                               sender_coherence);
    ++channel.messages_sent;
    ++total_messages_sent_;

    if (config_.log_messages) {
      std::cout << "    IPC: PID " << from_pid << " -> PID " << to_pid
                << " sent at tick=" << tick << " cycle=" << (int)sender_pos
                << " C=" << sender_coherence << "\n";
    }

    return true;
  }

  std::vector<Message> receive_messages(uint32_t to_pid, uint32_t from_pid,
                                        uint64_t tick, uint8_t receiver_pos,
                                        double receiver_coherence) {
    std::vector<Message> delivered;

    if (config_.enable_coherence_check) {
      if (receiver_coherence < config_.coherence_threshold) {
        return delivered;
      }
    }

    auto channel_it = find_channel(from_pid, to_pid);
    if (channel_it == channels_.end()) return delivered;

    auto &channel = *channel_it;
    auto it = channel.queue.begin();
    while (it != channel.queue.end()) {
      const auto &msg = *it;

      if (config_.enable_cycle_alignment) {
        if (receiver_pos != msg.sender_cycle_pos) {
          ++it;
          continue;
        }
      }

      delivered.push_back(msg);
      ++channel.messages_delivered;
      ++total_messages_delivered_;
      it = channel.queue.erase(it);
    }

    return delivered;
  }

  size_t pending_count(uint32_t from_pid, uint32_t to_pid) const {
    auto channel_it = find_channel(from_pid, to_pid);
    return channel_it != channels_.end() ? channel_it->queue.size() : 0;
  }

  bool has_channel(uint32_t from_pid, uint32_t to_pid) const {
    return find_channel(from_pid, to_pid) != channels_.end();
  }

  struct Stats {
    uint64_t total_sent;
    uint64_t total_delivered;
    uint64_t blocked_sends;
    size_t active_channels;
    size_t total_pending;
  };

  Stats get_stats() const {
    size_t pending = 0;
    for (const auto &ch : channels_) pending += ch.queue.size();
    return Stats{total_messages_sent_, total_messages_delivered_,
                 blocked_sends_, channels_.size(), pending};
  }

  void report_stats() const {
    auto stats = get_stats();
    std::cout << "  IPC: " << stats.total_sent << " sent, "
              << stats.total_delivered << " delivered, " << stats.blocked_sends
              << " blocked, " << stats.active_channels << " channels, "
              << stats.total_pending << " pending\n";
  }

  void reset_stats() {
    total_messages_sent_ = 0;
    total_messages_delivered_ = 0;
    blocked_sends_ = 0;
    channels_.clear();
  }

  Config config_;

private:
  std::vector<Channel> channels_;
  uint64_t total_messages_sent_ = 0;
  uint64_t total_messages_delivered_ = 0;
  uint64_t blocked_sends_ = 0;

  std::vector<Channel>::iterator find_channel(uint32_t from_pid, uint32_t to_pid) {
    return std::find_if(channels_.begin(), channels_.end(),
                        [from_pid, to_pid](const Channel &ch) {
                          return ch.sender_pid == from_pid &&
                                 ch.receiver_pid == to_pid;
                        });
  }

  std::vector<Channel>::const_iterator find_channel(uint32_t from_pid,
                                                    uint32_t to_pid) const {
    return std::find_if(channels_.begin(), channels_.end(),
                        [from_pid, to_pid](const Channel &ch) {
                          return ch.sender_pid == from_pid &&
                                 ch.receiver_pid == to_pid;
                        });
  }

  Channel &get_or_create_channel(uint32_t from_pid, uint32_t to_pid) {
    auto it = find_channel(from_pid, to_pid);
    if (it != channels_.end()) return *it;
    channels_.emplace_back(from_pid, to_pid);
    return channels_.back();
  }
};

} // namespace kernel::scheduling::ipc
