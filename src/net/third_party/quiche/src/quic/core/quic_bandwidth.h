// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// QuicBandwidth represents a bandwidth, stored in bits per second resolution.

#ifndef QUICHE_QUIC_CORE_QUIC_BANDWIDTH_H_
#define QUICHE_QUIC_CORE_QUIC_BANDWIDTH_H_

#include <cmath>
#include <cstdint>
#include <limits>
#include <ostream>
#include <string>

#include "net/third_party/quiche/src/quic/core/quic_constants.h"
#include "net/third_party/quiche/src/quic/core/quic_time.h"
#include "net/third_party/quiche/src/quic/core/quic_types.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_export.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_flag_utils.h"

namespace quic {

class QUIC_EXPORT_PRIVATE QuicBandwidth {
 public:
  // Creates a new QuicBandwidth with an internal value of 0.
  static constexpr QuicBandwidth Zero() { return QuicBandwidth(0); }

  // Creates a new QuicBandwidth with an internal value of INT64_MAX.
  static constexpr QuicBandwidth Infinite() {
    return QuicBandwidth(std::numeric_limits<int64_t>::max());
  }

  // Create a new QuicBandwidth holding the bits per second.
  static constexpr QuicBandwidth FromBitsPerSecond(int64_t bits_per_second) {
    return QuicBandwidth(bits_per_second);
  }

  // Create a new QuicBandwidth holding the kilo bits per second.
  static constexpr QuicBandwidth FromKBitsPerSecond(int64_t k_bits_per_second) {
    return QuicBandwidth(k_bits_per_second * 1000);
  }

  // Create a new QuicBandwidth holding the bytes per second.
  static constexpr QuicBandwidth FromBytesPerSecond(int64_t bytes_per_second) {
    return QuicBandwidth(bytes_per_second * 8);
  }

  // Create a new QuicBandwidth holding the kilo bytes per second.
  static constexpr QuicBandwidth FromKBytesPerSecond(
      int64_t k_bytes_per_second) {
    return QuicBandwidth(k_bytes_per_second * 8000);
  }

  // Create a new QuicBandwidth based on the bytes per the elapsed delta.
  static inline QuicBandwidth FromBytesAndTimeDelta(QuicByteCount bytes,
                                                    QuicTime::Delta delta) {
    if (!GetQuicReloadableFlag(quic_round_up_tiny_bandwidth)) {
      return QuicBandwidth((8 * bytes * kNumMicrosPerSecond) /
                           delta.ToMicroseconds());
    }

    if (bytes == 0) {
      return QuicBandwidth(0);
    }

    // 1 bit is 1000000 micro bits.
    int64_t num_micro_bits = 8 * bytes * kNumMicrosPerSecond;
    if (num_micro_bits < delta.ToMicroseconds()) {
      QUIC_RELOADABLE_FLAG_COUNT(quic_round_up_tiny_bandwidth);
      return QuicBandwidth(1);
    }

    return QuicBandwidth(num_micro_bits / delta.ToMicroseconds());
  }

  inline int64_t ToBitsPerSecond() const { return bits_per_second_; }

  inline int64_t ToKBitsPerSecond() const { return bits_per_second_ / 1000; }

  inline int64_t ToBytesPerSecond() const { return bits_per_second_ / 8; }

  inline int64_t ToKBytesPerSecond() const { return bits_per_second_ / 8000; }

  inline QuicByteCount ToBytesPerPeriod(QuicTime::Delta time_period) const {
    return bits_per_second_ * time_period.ToMicroseconds() / 8 /
           kNumMicrosPerSecond;
  }

  inline int64_t ToKBytesPerPeriod(QuicTime::Delta time_period) const {
    return bits_per_second_ * time_period.ToMicroseconds() / 8000 /
           kNumMicrosPerSecond;
  }

  inline bool IsZero() const { return bits_per_second_ == 0; }
  inline bool IsInfinite() const {
    return bits_per_second_ == Infinite().ToBitsPerSecond();
  }

  inline QuicTime::Delta TransferTime(QuicByteCount bytes) const {
    if (bits_per_second_ == 0) {
      return QuicTime::Delta::Zero();
    }
    return QuicTime::Delta::FromMicroseconds(bytes * 8 * kNumMicrosPerSecond /
                                             bits_per_second_);
  }

  std::string ToDebuggingValue() const;

 private:
  explicit constexpr QuicBandwidth(int64_t bits_per_second)
      : bits_per_second_(bits_per_second >= 0 ? bits_per_second : 0) {}

  int64_t bits_per_second_;

  friend QuicBandwidth operator+(QuicBandwidth lhs, QuicBandwidth rhs);
  friend QuicBandwidth operator-(QuicBandwidth lhs, QuicBandwidth rhs);
  friend QuicBandwidth operator*(QuicBandwidth lhs, float factor);
};

// Non-member relational operators for QuicBandwidth.
inline bool operator==(QuicBandwidth lhs, QuicBandwidth rhs) {
  return lhs.ToBitsPerSecond() == rhs.ToBitsPerSecond();
}
inline bool operator!=(QuicBandwidth lhs, QuicBandwidth rhs) {
  return !(lhs == rhs);
}
inline bool operator<(QuicBandwidth lhs, QuicBandwidth rhs) {
  return lhs.ToBitsPerSecond() < rhs.ToBitsPerSecond();
}
inline bool operator>(QuicBandwidth lhs, QuicBandwidth rhs) {
  return rhs < lhs;
}
inline bool operator<=(QuicBandwidth lhs, QuicBandwidth rhs) {
  return !(rhs < lhs);
}
inline bool operator>=(QuicBandwidth lhs, QuicBandwidth rhs) {
  return !(lhs < rhs);
}

// Non-member arithmetic operators for QuicBandwidth.
inline QuicBandwidth operator+(QuicBandwidth lhs, QuicBandwidth rhs) {
  return QuicBandwidth(lhs.bits_per_second_ + rhs.bits_per_second_);
}
inline QuicBandwidth operator-(QuicBandwidth lhs, QuicBandwidth rhs) {
  return QuicBandwidth(lhs.bits_per_second_ - rhs.bits_per_second_);
}
inline QuicBandwidth operator*(QuicBandwidth lhs, float rhs) {
  return QuicBandwidth(
      static_cast<int64_t>(std::llround(lhs.bits_per_second_ * rhs)));
}
inline QuicBandwidth operator*(float lhs, QuicBandwidth rhs) {
  return rhs * lhs;
}
inline QuicByteCount operator*(QuicBandwidth lhs, QuicTime::Delta rhs) {
  return lhs.ToBytesPerPeriod(rhs);
}
inline QuicByteCount operator*(QuicTime::Delta lhs, QuicBandwidth rhs) {
  return rhs * lhs;
}

// Override stream output operator for gtest.
inline std::ostream& operator<<(std::ostream& output,
                                const QuicBandwidth bandwidth) {
  output << bandwidth.ToDebuggingValue();
  return output;
}

}  // namespace quic
#endif  // QUICHE_QUIC_CORE_QUIC_BANDWIDTH_H_
