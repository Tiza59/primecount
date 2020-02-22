///
/// @file  pi_lmo5.cpp
/// @brief Implementation of the Lagarias-Miller-Odlyzko prime
///        counting algorithm. This version uses the modified
///        algorithm as described in section 5 (pages 556-557) in the
///        paper "Computing pi(x) The Meissel-Lehmer Method",
///        Mathematics of Computation, 44 (1985), by J. C. Lagarias,
///        V. S. Miller and A. M. Odlyzko.
///
///        Unlike pi_lmo4.cpp this version does not use a special tree
///        data structure (a.k.a. Fenwick tree) for counting the number
///        of unsieved elements but instead counts the number of
///        unsieved elements directly from the sieve array using the
///        POPCNT instruction which is much faster.
///
///        Lagarias-Miller-Odlyzko formula:
///        pi(x) = pi(y) + S1(x, a) + S2(x, a) - 1 - P2(x, a)
///        with y = x^(1/3), a = pi(y)
///
/// Copyright (C) 2020 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primecount-internal.hpp>
#include <Sieve.hpp>
#include <generate.hpp>
#include <min.hpp>
#include <imath.hpp>
#include <PhiTiny.hpp>
#include <print.hpp>
#include <S.hpp>

#include <stdint.h>
#include <vector>

using namespace std;
using namespace primecount;

namespace {

/// Calculate the contribution of the special leaves
int64_t S2(int64_t x,
           int64_t y,
           int64_t c,
           vector<int32_t>& primes,
           vector<int32_t>& lpf,
           vector<int32_t>& mu)
{
  print("");
  print("=== S2(x, y) ===");
  print("Computation of the special leaves");

  double time = get_time();
  int64_t limit = x / y;
  int64_t segment_size = Sieve::get_segment_size(isqrt(limit));
  int64_t low = 0;

  double alpha = get_alpha(x, y);
  Sieve sieve(low, segment_size, primes.size(), alpha);
  vector<int32_t> pi = generate_pi(y);
  vector<int64_t> phi(primes.size(), 0);

  int64_t s2 = 0;
  int64_t pi_sqrty = pi[isqrt(y)];
  int64_t pi_y = pi[y];

  // segmented sieve of Eratosthenes
  for (; low < limit; low += segment_size)
  {
    // current segment [low, high[
    int64_t high = min(low + segment_size, limit);
    int64_t low1 = max(low, 1);

    sieve.pre_sieve(primes, c, low, high);
    int64_t b = c + 1;

    // For c + 1 <= b <= pi_sqrty
    // Find all special leaves in the current segment that are
    // composed of a prime and a square free number:
    // low <= x / (primes[b] * m) < high
    for (; b <= pi_sqrty; b++)
    {
      int64_t prime = primes[b];
      int64_t min_m = max(x / (prime * high), y / prime);
      int64_t max_m = min(x / (prime * low1), y);

      if (prime >= max_m)
        goto next_segment;

      for (int64_t m = max_m; m > min_m; m--)
      {
        if (mu[m] != 0 && prime < lpf[m])
        {
          int64_t xpm = x / (prime * m);
          int64_t stop = xpm - low;
          int64_t phi_xpm = phi[b] + sieve.count(stop);
          s2 -= mu[m] * phi_xpm;
        }
      }

      phi[b] += sieve.get_total_count();
      sieve.cross_off_count(prime, b);
    }

    // For pi_sqrty < b < pi_y
    // Find all special leaves in the current segment
    // that are composed of 2 primes:
    // low <= x / (primes[b] * primes[l]) < high
    for (; b < pi_y; b++)
    {
      int64_t prime = primes[b];
      int64_t l = pi[min(x / (prime * low1), y)];
      int64_t min_m = max(x / (prime * high), prime);

      if (prime >= primes[l])
        goto next_segment;

      for (; primes[l] > min_m; l--)
      {
        int64_t xpq = x / (prime * primes[l]);
        int64_t stop = xpq - low;
        int64_t phi_xpq = phi[b] + sieve.count(stop);
        s2 += phi_xpq;
      }

      phi[b] += sieve.get_total_count();
      sieve.cross_off_count(prime, b);
    }

    next_segment:;
  }

  print("S2", s2, time);
  return s2;
}

} // namespace

namespace primecount {

/// Calculate the number of primes below x using the
/// Lagarias-Miller-Odlyzko algorithm.
/// Run time: O(x^(2/3) / log x)
/// Memory usage: O(x^(1/3) * (log x)^2)
///
int64_t pi_lmo5(int64_t x)
{
  if (x < 2)
    return 0;

  double alpha = get_alpha_lmo(x);
  int64_t x13 = iroot<3>(x);
  int64_t y = (int64_t) (x13 * alpha);
  int64_t z = x / y;
  int64_t c = PhiTiny::get_c(y);

  print("");
  print("=== pi_lmo5(x) ===");
  print("pi(x) = S1 + S2 + pi(y) - 1 - P2");
  print(x, y, z, c, 1);

  int64_t p2 = P2(x, y, 1);
  auto primes = generate_primes<int32_t>(y);
  auto lpf = generate_lpf(y);
  auto mu = generate_moebius(y);

  int64_t pi_y = primes.size() - 1;
  int64_t s1 = S1(x, y, c, 1);
  int64_t s2 = S2(x, y, c, primes, lpf, mu);
  int64_t phi = s1 + s2;
  int64_t sum = phi + pi_y - 1 - p2;

  return sum;
}

} // namespace
