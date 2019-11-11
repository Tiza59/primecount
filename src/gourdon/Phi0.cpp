///
/// @file  Phi0.cpp
/// @brief Calculate the contribution of the ordinary leaves in
///        Xavier Gourdon's prime counting algorithm. Note that the
///        Phi0 formula on the 7th page of Gourdon's paper is not
///        correct, however the correct Phi0 formula can be found on
///        page 3 of Gourdon's paper.
///
///        The only difference in the computation of the ordinary
///        leaves compared to the Deleglise-Rivat algorithm is that we
///        iterate over all square free numbers <= z (instead of y)
///        whose largest prime factor <= y, with z >= y and
///        z < x^(1/2). Also the small constant is named k instead
///        of c.
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <gourdon.hpp>
#include <primecount-internal.hpp>
#include <PhiTiny.hpp>
#include <generate.hpp>
#include <imath.hpp>
#include <int128_t.hpp>
#include <json.hpp>
#include <backup.hpp>
#include <print.hpp>

#include <stdint.h>
#include <vector>

using namespace std;
using namespace primecount;

namespace {

void backup(maxint_t x,
            int64_t y,
            int64_t z,
            int64_t k,
            maxint_t sum,
            double time)
{
  auto json = load_backup();

  json["Phi0"]["x"] = to_string(x);
  json["Phi0"]["y"] = y;
  json["Phi0"]["z"] = z;
  json["Phi0"]["k"] = k;
  json["Phi0"]["alpha_y"] = get_alpha_y(x, y);
  json["Phi0"]["alpha_z"] = get_alpha_z(y, z);
  json["Phi0"]["sum"] = to_string(sum);
  json["Phi0"]["percent"] = 100.0;
  json["Phi0"]["seconds"] = get_time() - time;

  store_backup(json);
}

template <typename T>
bool resume(T x,
            int64_t y,
            int64_t z,
            int64_t k,
            T& sum,
            double& time)
{
  auto json = load_backup();

  if (is_resume(json, "Phi0", x, y, z, k))
  {
    double percent = json["Phi0"]["percent"];
    double seconds = json["Phi0"]["seconds"];

    sum = (T) to_maxint(json["Phi0"]["sum"]);
    time = get_time() - seconds;
    print_resume(percent, x);
    return true;
  }

  return false;
}

/// Recursively iterate over the square free numbers coprime
/// to the first b primes and calculate the sum of the
/// ordinary leaves. This algorithm is described in section
/// 2.2 of the paper: Douglas Staple, "The Combinatorial
/// Algorithm For Computing pi(x)", arXiv:1503.01839, 6 March
/// 2015.
///
template <int MU, typename T, typename P>
T Phi0_thread(T x,
              int64_t z,
              uint64_t b,
              int64_t k,
              T square_free,
              vector<P>& primes)
{
  T phi0 = 0;

  for (b++; b < primes.size(); b++)
  {
    T next = square_free * primes[b];
    if (next > z) break;
    phi0 += MU * phi_tiny(x / next, k);
    phi0 += Phi0_thread<-MU>(x, z, b, k, next, primes);
  }

  return phi0;
}

/// Parallel computation of the ordinary leaves.
/// Run time: O(z)
/// Memory usage: O(y / log(y))
///
template <typename X, typename Y>
X Phi0_OpenMP(X x,
              Y y,
              int64_t z,
              int64_t k,
              int threads)
{
  threads = ideal_num_threads(threads, y);

  auto primes = generate_primes<Y>(y);
  int64_t pi_y = primes.size() - 1;
  X phi0 = phi_tiny(x, k);

  #pragma omp parallel for schedule(static, 1) num_threads(threads) reduction (+: phi0)
  for (int64_t b = k + 1; b <= pi_y; b++)
  {
    phi0 -= phi_tiny(x / primes[b], k);
    phi0 += Phi0_thread<1>(x, z, b, k, (X) primes[b], primes);
  }

  return phi0;
}

} // namespace

namespace primecount {

int64_t Phi0(int64_t x,
             int64_t y,
             int64_t z,
             int64_t k,
             int threads)
{
  print("");
  print("=== Phi0(x, y) ===");
  print_gourdon_vars(x, y, z, k, threads);

  double time = get_time();
  int64_t phi0 = 0;

  if (!resume(x, y, z, k, phi0, time))
  {
    phi0 = Phi0_OpenMP(x, y, z, k, threads);
    backup(x, y, z, k, phi0, time);
  }

  print("Phi0", phi0, time);
  return phi0;
}

#ifdef HAVE_INT128_T

int128_t Phi0(int128_t x,
              int64_t y,
              int64_t z,
              int64_t k,
              int threads)
{
  print("");
  print("=== Phi0(x, y) ===");
  print_gourdon_vars(x, y, z, k, threads);

  double time = get_time();
  int128_t phi0 = 0;

  if (!resume(x, y, z, k, phi0, time))
  {
    // uses less memory
    if (y <= numeric_limits<uint32_t>::max())
      phi0 = Phi0_OpenMP(x, (uint32_t) y, z, k, threads);
    else
      phi0 = Phi0_OpenMP(x, y, z, k, threads);

    backup(x, y, z, k, phi0, time);
  }

  print("Phi0", phi0, time);
  return phi0;
}

#endif

} // namespace
