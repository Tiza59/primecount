///
/// @file  MpiLoadBalancer.hpp
/// @brief The MpiLoadBalancer evenly distributes the computation
///        of the hard special leaves onto cluster nodes.
///
/// Copyright (C) 2020 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef MPILOADBALANCER_HPP
#define MPILOADBALANCER_HPP

#include <mpi.h>
#include <LoadBalancer.hpp>
#include <MpiMsg.hpp>
#include <S2Status.hpp>
#include <int128_t.hpp>
#include <stdint.h>

namespace primecount {

class MpiLoadBalancer
{
public:
  MpiLoadBalancer(maxint_t x, int64_t sieve_limit, maxint_t sum_approx);
  void get_work(MpiMsg* msg);

private:
  void update_segments(ThreadSettings& thread);
  double remaining_secs() const;

  int64_t low_;
  int64_t max_low_;
  int64_t sieve_limit_;
  int64_t segments_;
  int64_t segment_size_;
  int64_t max_size_;
  maxint_t sum_;
  maxint_t sum_approx_;
  double time_;
  S2Status status_;
};

} // namespace

#endif
