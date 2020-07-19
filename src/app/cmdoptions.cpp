///
/// @file   cmdoptions.cpp
/// @brief  Parse command-line options for the primecount console
///         (terminal) application.
///
/// Copyright (C) 2020 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include "cmdoptions.hpp"

#include <primecount.hpp>
#include <primecount-internal.hpp>
#include <print.hpp>
#include <int128_t.hpp>

#include <stdint.h>
#include <fstream>
#include <cstddef>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
#include <utility>

using namespace std;

namespace primecount {

// backup.cpp
std::string backup_file();
void set_backup_file(const std::string& backup_file);

// help.cpp
void help(int exitCode);
void version();
void test();

/// Some command-line options require an additional parameter.
/// Examples: --threads THREADS, -a ALPHA, ...
enum IsParam
{
  NO_PARAM,
  REQUIRED_PARAM,
  OPTIONAL_PARAM
};

/// Command-line options
map<string, std::pair<OptionID, IsParam>> optionMap =
{
  { "-b", make_pair(OPTION_BACKUP, REQUIRED_PARAM) },
  { "--backup", make_pair(OPTION_BACKUP, REQUIRED_PARAM) },
  { "-r", make_pair(OPTION_RESUME, OPTIONAL_PARAM) },
  { "--resume", make_pair(OPTION_RESUME, OPTIONAL_PARAM) },
  { "--alpha-y", make_pair(OPTION_ALPHA_Y, REQUIRED_PARAM) },
  { "--alpha-z", make_pair(OPTION_ALPHA_Z, REQUIRED_PARAM) },
  { "-g", make_pair(OPTION_GOURDON, NO_PARAM) },
  { "--gourdon", make_pair(OPTION_GOURDON, NO_PARAM) },
  { "--gourdon-64", make_pair(OPTION_GOURDON_64, NO_PARAM) },
  { "--gourdon-128", make_pair(OPTION_GOURDON_128, NO_PARAM) },
  { "-h", make_pair(OPTION_HELP, NO_PARAM) },
  { "--help", make_pair(OPTION_HELP, NO_PARAM) },
  { "-l", make_pair(OPTION_LEGENDRE, NO_PARAM) },
  { "--legendre", make_pair(OPTION_LEGENDRE, NO_PARAM) },
  { "-m", make_pair(OPTION_MEISSEL, NO_PARAM) },
  { "--meissel", make_pair(OPTION_MEISSEL, NO_PARAM) },
  { "-n", make_pair(OPTION_NTHPRIME, NO_PARAM) },
  { "--nth-prime", make_pair(OPTION_NTHPRIME, NO_PARAM) },
  { "--number", make_pair(OPTION_NUMBER, REQUIRED_PARAM) },
  { "-p", make_pair(OPTION_PRIMESIEVE, NO_PARAM) },
  { "--primesieve", make_pair(OPTION_PRIMESIEVE, NO_PARAM) },
  { "--Li", make_pair(OPTION_LI, NO_PARAM) },
  { "--Li-inverse", make_pair(OPTION_LIINV, NO_PARAM) },
  { "--Ri", make_pair(OPTION_RI, NO_PARAM) },
  { "--Ri-inverse", make_pair(OPTION_RIINV, NO_PARAM) },
  { "--phi", make_pair(OPTION_PHI, NO_PARAM) },
  { "--AC", make_pair(OPTION_AC, NO_PARAM) },
  { "-B", make_pair(OPTION_B, NO_PARAM) },
  { "--B", make_pair(OPTION_B, NO_PARAM) },
  { "-D", make_pair(OPTION_D, NO_PARAM) },
  { "--D", make_pair(OPTION_D, NO_PARAM) },
  { "--Phi0", make_pair(OPTION_PHI0, NO_PARAM) },
  { "--Sigma", make_pair(OPTION_SIGMA, NO_PARAM) },
  { "-s", make_pair(OPTION_STATUS, OPTIONAL_PARAM) },
  { "--status", make_pair(OPTION_STATUS, OPTIONAL_PARAM) },
  { "--test", make_pair(OPTION_TEST, NO_PARAM) },
  { "--time", make_pair(OPTION_TIME, NO_PARAM) },
  { "-t", make_pair(OPTION_THREADS, REQUIRED_PARAM) },
  { "--threads", make_pair(OPTION_THREADS, REQUIRED_PARAM) },
  { "-v", make_pair(OPTION_VERSION, NO_PARAM) },
  { "--version", make_pair(OPTION_VERSION, NO_PARAM) }
};

/// Command-line option
struct Option
{
  // Example:
  // str = "--threads=32"
  // opt = "--threads"
  // val = "32"
  string str;
  string opt;
  string val;

  template <typename T>
  T to() const
  {
    try {
      if (std::is_floating_point<T>::value)
        return (T) stod(val);
      else
        return (T) to_maxint(val);
    }
    catch (std::exception&) {
      throw primecount_error("invalid option '" + opt + "=" + val + "'");
    }
  }
};

/// Options start with "-" or "--", then
/// follows a Latin ASCII character.
///
bool isOption(const string& str)
{
  // Option of type: -o...
  if (str.size() >= 2 &&
      str[0] == '-' &&
      ((str[1] >= 'a' && str[1] <= 'z') ||
       (str[1] >= 'A' && str[1] <= 'Z')))
    return true;

  // Option of type: --o...
  if (str.size() >= 3 &&
      str[0] == '-' &&
      str[1] == '-' &&
      ((str[2] >= 'a' && str[2] <= 'z') ||
       (str[2] >= 'A' && str[2] <= 'Z')))
    return true;

  return false;
}

void optionStatus(Option& opt,
                  CmdOptions& opts)
{
  set_print(true);
  opts.time = true;

  if (!opt.val.empty())
    set_status_precision(opt.to<int>());
}

void optionResume(Option& opt,
                  CmdOptions& opts)
{
  if (!opt.val.empty())
    opts.resumeFile = opt.val;
  else
    opts.resumeFile = backup_file();

  set_backup_file(opts.resumeFile);
  ifstream ifs(backup_file());

  if (!ifs.is_open())
    throw primecount_error("failed to open backup file: " + backup_file());
}

/// Parse the next command-line option.
/// e.g. "--threads=32"
/// -> opt.str = "--threads=32"
/// -> opt.opt = "--threads"
/// -> opt.val = "8"
///
Option parseOption(int argc, char* argv[], int& i)
{
  Option opt;
  opt.str = argv[i];

  if (opt.str.empty())
    throw primecount_error("unrecognized option ''");

  // Check if the option has the format:
  // --opt or -o (but not --opt=N)
  if (optionMap.count(opt.str))
  {
    opt.opt = opt.str;
    IsParam isParam = optionMap[opt.str].second;

    if (isParam == REQUIRED_PARAM)
    {
      i += 1;

      if (i < argc)
        opt.val = argv[i];

      // Prevent --threads --other-option
      if (opt.val.empty() || isOption(opt.val))
        throw primecount_error("missing value for option '" + opt.opt + "'");
    }

    // If the option takes an optional argument we
    // assume the next value is an optional argument
    // if the value is not a vaild option.
    if (isParam == OPTIONAL_PARAM &&
        i + 1 < argc &&
        !string(argv[i + 1]).empty() &&
        !isOption(argv[i + 1]))
    {
      i += 1;
      opt.val = argv[i];
    }
  }
  else
  {
    // Here the option is either:
    // 1) An option of type: --opt[=N]
    // 2) An option of type: --opt[N]
    // 3) A number (e.g. the start number)

    if (isOption(opt.str))
    {
      size_t pos = opt.str.find("=");

      // Option of type: --opt=N
      if (pos != string::npos)
      {
        opt.opt = opt.str.substr(0, pos);
        opt.val = opt.str.substr(pos + 1);

        // Print partial option: --opt (without =N)
        if (!optionMap.count(opt.opt))
          throw primecount_error("unrecognized option '" + opt.opt + "'");
      }
      else
      {
        // Option of type: --opt[N]
        pos = opt.str.find_first_of("0123456789");

        if (pos == string::npos)
          opt.opt = opt.str;
        else
        {
          opt.opt = opt.str.substr(0, pos);
          opt.val = opt.str.substr(pos);
        }

        // Print full option e.g.: --opt123
        if (!optionMap.count(opt.opt))
          throw primecount_error("unrecognized option '" + opt.str + "'");
      }

      // Prevent '--option='
      if (opt.val.empty() &&
          optionMap[opt.opt].second == REQUIRED_PARAM)
        throw primecount_error("missing value for option '" + opt.opt + "'");
    }
    else
    {
      // Here the option is actually a number or
      // an integer arithmetic expression.
      opt.opt = "--number";
      opt.val = opt.str;

      // This is not a valid number
      if (opt.str.find_first_of("0123456789") == string::npos)
        throw primecount_error("unrecognized option '" + opt.str + "'");

      // Prevent negative numbers as there are
      // no negative prime numbers.
      if (opt.str.at(0) == '-')
        throw primecount_error("unrecognized option '" + opt.str + "'");
    }
  }

  return opt;
}

CmdOptions parseOptions(int argc, char* argv[])
{
  CmdOptions opts;
  vector<maxint_t> numbers;

  // No command-line options provided
  if (argc <= 1)
    help(/* exitCode */ 1);

  for (int i = 1; i < argc; i++)
  {
    Option opt = parseOption(argc, argv, i);
    OptionID optionID = optionMap[opt.opt].first;

    switch (optionID)
    {
      case OPTION_BACKUP:  set_backup_file(opt.val); break;
      case OPTION_RESUME:  optionResume(opt, opts); break;
      case OPTION_ALPHA_Y: set_alpha_y(opt.to<double>()); break;
      case OPTION_ALPHA_Z: set_alpha_z(opt.to<double>()); break;
      case OPTION_NUMBER:  numbers.push_back(opt.to<maxint_t>()); break;
      case OPTION_THREADS: set_num_threads(opt.to<int>()); break;
      case OPTION_HELP:    help(/* exitCode */ 0); break;
      case OPTION_STATUS:  optionStatus(opt, opts); break;
      case OPTION_TIME:    opts.time = true; break;
      case OPTION_TEST:    test(); break;
      case OPTION_VERSION: version(); break;
      default:             opts.option = optionID;
    }
  }

  if (!opts.is_resume())
  {
    if (opts.option == OPTION_PHI)
    {
      if (numbers.size() < 2)
        throw primecount_error("option --phi requires 2 numbers");
      opts.a = numbers[1];
    }

    if (numbers.empty())
      throw primecount_error("missing x number");
    opts.x = numbers[0];
  }

  if (!opts.backupFile.empty() &&
      !opts.resumeFile.empty() &&
      opts.backupFile != opts.resumeFile)
  {
    throw primecount_error("resume and backup file must be identical");
  }

  return opts;
}

} // namespace
