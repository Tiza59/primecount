///
/// @file  print.hpp
///
/// Copyright (C) 2020 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef PRINT_HPP
#define PRINT_HPP

#include <int128_t.hpp>

#include <stdint.h>
#include <string>

namespace primecount {

void set_print(bool print);
void set_print_variables(bool print_variables);

bool is_print();
bool is_print_combined_result();
void print_seconds(double seconds);
void print_status(double percent, maxint_t x);
void print_resume(double percent, maxint_t x);

void print(const std::string& str);
void print(const std::string& str, maxint_t res);
void print(const std::string& str, maxint_t res, double time);
void print_vars(maxint_t x, int64_t y, int threads);

void print_gourdon(maxint_t x, int64_t y, int64_t z, int64_t k, int threads);
void print_gourdon_vars(maxint_t x, int64_t y, int threads);
void print_gourdon_vars(maxint_t x, int64_t y, int64_t z, int64_t k,  int threads);

} // namespace

#endif
