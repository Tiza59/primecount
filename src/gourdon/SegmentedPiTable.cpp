///
/// @file  SegmentedPiTable.cpp
/// @brief The A and C formulas in Xavier Gourdon's prime counting
///        algorithm require looking up PrimePi[n] values with
///        n < x^(1/2). Since a PrimePi[n] lookup table of size
///        x^(1/2) would use too much memory we need a segmented
///        PrimePi[n] lookup table that uses only O(z) memory.
///
///        The SegmentedPiTable is based on the PiTable class which
///        is a compressed lookup table for prime counts. Each bit
///        in the lookup table corresponds to an odd integer and that
///        bit is set to 1 if the integer is a prime. PiTable uses
///        only (n / 8) bytes of memory and returns the number of
///        primes <= n in O(1) operations.
///
/// Copyright (C) 2021 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <SegmentedPiTable.hpp>
#include <primecount-internal.hpp>
#include <primesieve.hpp>
#include <imath.hpp>
#include <min.hpp>

#include <stdint.h>
#include <cstring>

namespace primecount {

SegmentedPiTable::SegmentedPiTable(uint64_t low,
                                   uint64_t limit,
                                   uint64_t segment_size,
                                   int threads)
  : counts_(threads),
    low_(low),
    max_high_(limit + 1),
    threads_(threads)
{
  // Each bit of the pi[x] lookup table corresponds
  // to an odd integer, so there are 16 numbers per
  // byte. However we also store 64-bit prime_count
  // values in the pi[x] lookup table, hence each byte
  // only corresponds to 8 numbers.
  uint64_t numbers_per_byte = 8;

  // Minimum segment size = 256 KiB (L2 cache size),
  // a large segment size improves load balancing.
  uint64_t min_segment_size = 256 * (1 << 10) * numbers_per_byte;
  segment_size_ = max(segment_size, min_segment_size);
  segment_size_ = min(segment_size_, max_high_);

  // In order to simplify multi-threading we set low,
  // high and segment_size % 128 == 0.
  segment_size_ += 128 - segment_size_ % 128;
  int thread_threshold = (int) 1e7;
  threads_ = ideal_num_threads(threads, segment_size_, thread_threshold);

  high_ = low_ + segment_size_;
  high_ = std::min(high_, max_high_);
  pi_.resize(segment_size_ / 128);

  low = max(low, 1);
  pi_low_ = pi_simple(low - 1, threads);
  init();
}

/// Increase low & high and initialize the next segment.
void SegmentedPiTable::next()
{
  // pi_low_ must be initialized before updating the
  // member variables for the next segment.
  pi_low_ = operator[](high_ - 1);

  low_ = high_;
  high_ = low_ + segment_size_;
  high_ = std::min(high_, max_high_);

  if (finished())
    return;

  init();
}

/// Iterate over the primes inside the segment [low, high[
/// and initialize the pi[x] lookup table. The pi[x]
/// lookup table returns the number of primes <= x for
/// low <= x < high.
///
void SegmentedPiTable::init()
{
  uint64_t thread_size = segment_size_ / threads_;
  uint64_t min_thread_size = (uint64_t) 1e7;
  thread_size = max(min_thread_size, thread_size);
  thread_size += 128 - thread_size % 128;

  #pragma omp parallel num_threads(threads_)
  {
    #pragma omp for
    for (int t = 0; t < threads_; t++)
    {
      uint64_t start = low_ + thread_size * t;
      uint64_t stop = start + thread_size;
      stop = min(stop, high_);

      if (start < stop)
        init_bits(start, stop, t);
    }

    #pragma omp for
    for (int t = 0; t < threads_; t++)
    {
      uint64_t start = low_ + thread_size * t;
      uint64_t stop = start + thread_size;
      stop = min(stop, high_);

      if (start < stop)
        init_prime_count(start, stop, t);
    }
  }
}

/// Each thread computes PrimePi [start, stop[
void SegmentedPiTable::init_bits(uint64_t start,
                                 uint64_t stop,
                                 uint64_t thread_num)
{
  // Zero initialize pi vector
  uint64_t i = (start - low_) / 128;
  uint64_t j = ceil_div((stop - low_), 128);
  std::memset(&pi_[i], 0, (j - i) * sizeof(pi_t));

  // Since we store only odd numbers in our lookup table,
  // we cannot store 2 which is the only even prime.
  // As a workaround we mark 1 as a prime (1st bit) and
  // add a check to return 0 for pi[1].
  if (start <= 1)
    pi_[0].bits |= 1;

  // Iterate over primes > 2
  primesieve::iterator it(max(start, 2), stop);
  uint64_t count = (start <= 2);
  uint64_t prime = 0;

  // Each thread iterates over the primes
  // inside [start, stop[ and initializes
  // the pi[x] lookup table.
  while ((prime = it.next_prime()) < stop)
  {
    uint64_t p = prime - low_;
    uint64_t prime_bit = 1ull << (p % 128 / 2);
    pi_[p / 128].bits |= prime_bit;
    count += 1;
  }

  counts_[thread_num] = count;
}

/// Each thread computes PrimePi [start, stop[
void SegmentedPiTable::init_prime_count(uint64_t start,
                                        uint64_t stop,
                                        uint64_t thread_num)
{
  // First compute PrimePi[start - 1]
  uint64_t count = pi_low_;
  for (uint64_t i = 0; i < thread_num; i++)
    count += counts_[i];

  // Convert to array indexes
  uint64_t i = (start - low_) / 128;
  uint64_t stop_idx = ceil_div((stop - low_), 128);

  for (; i < stop_idx; i++)
  {
    pi_[i].prime_count = count;
    count += popcnt64(pi_[i].bits);
  }
}

} // namespace
