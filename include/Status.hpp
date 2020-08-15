///
/// @file  Status.hpp
///
/// Copyright (C) 2020 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef STATUS_HPP
#define STATUS_HPP

#include <int128_t.hpp>
#include <noinline.hpp>

namespace primecount {

class Status
{
public:
  Status(maxint_t x);
  NOINLINE void print(int64_t n, int64_t limit);
  NOINLINE void print(int64_t low, int64_t limit, maxint_t sum, maxint_t sum_approx);
  static double getPercent(int64_t low, int64_t limit, maxint_t sum, maxint_t sum_approx);
  void setPercent(double percent) { percent_ = percent; }
private:
  bool isPrint(double time);
  void print(double percent);
  double epsilon_;
  double percent_ = -1;
  double time_ = 0;
  double is_print_ = 1.0 / 20;
  int precision_;
};

} // namespace

#endif
