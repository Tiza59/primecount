///
/// @file  pi_lmo3.cpp
/// @brief Simple implementation of the Lagarias-Miller-Odlyzko prime
///        counting algorithm. This implementation uses the segmented
///        sieve of Eratosthenes to calculate S2(x).
///
/// Copyright (C) 2014 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include "PhiTiny.hpp"
#include "pmath.hpp"
#include "Pk.hpp"

#include <primecount.hpp>
#include <primesieve.hpp>
#include <stdint.h>
#include <vector>

using std::log;

namespace {

/// Calculate the contribution of the ordinary leaves.
///
int64_t S1(int64_t x,
           int64_t x13_alpha,
           int64_t c,
           std::vector<int32_t>& primes,
           std::vector<int32_t>& lpf,
           std::vector<int32_t>& mu)
{
  int64_t S1_result = 0;

  for (int64_t n = 1; n <= x13_alpha; n++)
    if (lpf[n] > primes[c])
      S1_result += mu[n] * primecount::phi(x / n, c);

  return S1_result;
}

/// Calculate the contribution of the special leaves.
/// This implementation uses segmentation which reduces the
/// algorithm's space complexity to O(n^(1/3)).
/// @pre c >= 2
///
int64_t S2(int64_t x,
           int64_t x13_alpha,
           int64_t x23_alpha,
           int64_t a,
           int64_t c,
           std::vector<int32_t>& primes,
           std::vector<int32_t>& lpf,
           std::vector<int32_t>& mu)
{
  int64_t limit = x23_alpha + 1;
  int64_t S2_result = 0;
  int64_t min_segment_size = 10;
  int64_t segment_size = std::max(min_segment_size, limit / x13_alpha);

  // vector used for sieving
  std::vector<char> sieve(segment_size);
  std::vector<int64_t> next(primes.begin(), primes.end());
  std::vector<int64_t> phi(primes.size(), 0);

  // segmented sieve of Eratosthenes
  for (int64_t low = 1; low < limit; low += segment_size)
  {
    std::fill(sieve.begin(), sieve.end(), 1);

    // current segment = interval [low, high[
    int64_t high = std::min(low + segment_size, limit);

    // phi(y, b) nodes with b <= c do not contribute to S2, so we
    // simply sieve out the multiples of the first c primes
    //
    for (int64_t b = 1; b <= c; b++)
    {
      int64_t k = next[b];
      for (int64_t prime = primes[b]; k < high; k += prime)
        sieve[k - low] = 0;
      next[b] = k;
    }

    for (int64_t b = c; b + 1 < a; b++)
    {
      int64_t prime = primes[b + 1];
      int64_t max_m = std::min(x / (low * prime), x13_alpha);
      int64_t min_m = std::max(x / (high * prime), x13_alpha / prime);
      int64_t i = low;

      if (prime > max_m)
        break;

      for (int64_t m = max_m; m > min_m; m--)
      {
        if (mu[m] != 0 && prime < lpf[m])
        {
          // We have found a special leaf, compute it's contribution
          // phi(x / (m * primes[b + 1]), b) by counting the
          // number of unsieved elements <= x / (m * primes[b + 1])
          // after having removed the multiples of the first b primes
          //
          for (int64_t y = x / (m * prime); i <= y; i++)
            phi[b + 1] += sieve[i - low];

          S2_result -= mu[m] * phi[b + 1];
        }
      }

      // Count the remaining unsieved elements in this segment,
      // we need their values in the next segment
      for (; i < high; i++)
        phi[b + 1] += sieve[i - low];

      // Remove the multiples of (b + 1)th prime
      int64_t k = next[b + 1];
      for (; k < high; k += prime * 2)
        sieve[k - low] = 0;
      next[b + 1] = k;
    }
  }

  return S2_result;
}

} // namespace

namespace primecount {

/// Calculate the number of primes below x using the
/// Lagarias-Miller-Odlyzko algorithm.
/// Run time: O(x^(2/3)) operations, O(x^0.5) space.
/// @note O(x^0.5) space is due to parallel P2(x, a).
///
int64_t pi_lmo3(int64_t x, int threads)
{
  if (x < 2)
    return 0;

  // Optimization factor, see:
  // Tomás Oliveira e Silva, Computing pi(x): the combinatorial method,
  // Revista do DETUA, vol. 4, no. 6, pp. 763-764, March 2006.
  double beta = 1.1;
  double alpha = std::max(1.0, log(log((double) x)) * beta);

  int64_t x13 = iroot<3>(x);
  int64_t x13_alpha = (int64_t)(x13 * alpha);
  int64_t x23_alpha = (int64_t) std::pow((double) x / alpha, 2.0 / 3.0);
  int64_t a = pi_lehmer(x13_alpha);
  int64_t c = (a < 6) ? a : 6;

  std::vector<int32_t> lpf = make_least_prime_factor(x13_alpha);
  std::vector<int32_t> mu = make_moebius(x13_alpha);
  std::vector<int32_t> primes;
  primes.push_back(0);
  primesieve::generate_n_primes(a, &primes);

  int64_t phi = S1(x, x13_alpha, c, primes, lpf , mu) + S2(x, x13_alpha, x23_alpha, a, c, primes, lpf , mu);
  int64_t sum = phi + a - 1 - P2(x, a, threads);

  return sum;
}

} // namespace primecount
