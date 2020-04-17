#pragma once

#include <fstream>
#include <string>

namespace sortnet {

  // these nice helper funcs (binary_read and binary_write) were inspired by:
  // https://baptiste-wicht.com/posts/2011/06/write-and-read-binary-files-in-c.html
  template <typename T> std::istream &binary_read(std::istream &f, T &value) {
    return f.read(reinterpret_cast<char *>(&value), sizeof(T));
  }

  template <typename T> std::ostream &binary_write(std::ostream &f, const T &value) {
    return f.write(reinterpret_cast<const char *>(&value), sizeof(T));
  }

  bool fileExists(const std::string &filename);
}