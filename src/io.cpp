#include "../app/src/io.h"

#include <sys/stat.h>

namespace sortnet {
  bool fileExists(const std::string &filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
  }
}  // namespace sortnet