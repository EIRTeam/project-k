#pragma once

#include <iostream>
#include <sstream>
#include <exception>

#include "../utility/stream.hpp"
#include "core/string/print_string.h"

namespace tf {

// Procedure: throw_se
// Throws the system error under a given error code.
template <typename... ArgsT>
//void throw_se(const char* fname, const size_t line, Error::Code c, ArgsT&&... args) {
void throw_re(const char* fname, const size_t line, ArgsT&&... args) {
  std::ostringstream oss;
  oss << "[" << fname << ":" << line << "] ";
  //ostreamize(oss, std::forward<ArgsT>(args)...);
  (oss << ... << args);
  std::string std_str = oss.str();
  std::cout << std_str << std::endl;
  String str = String(std_str.c_str());
  print_line("THROW!", str);
}

}  // ------------------------------------------------------------------------

#define TF_THROW(...) tf::throw_re(__FILE__, __LINE__, __VA_ARGS__);

