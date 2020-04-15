#pragma once

#include <string>

#include "../config/config.h"
#include "sortnet/vendors/github.com/prakhar1989/progress-cpp/ProgressBar.hpp"

class Progress {
 protected:
  const std::string prefix;
  ProgressBar bar;
  float_t multiplier{1};

  int prevProgress{-1};
  const unsigned int total_ticks;
  const unsigned int bar_width;
  unsigned int ticks = 0;

  void print() const {
    std::cout << prefix << " ";
  }

 public:
  Progress(const std::string prefix, uint64_t total, uint8_t width) : prefix(prefix), bar(total, width), total_ticks(total), bar_width(width) {}

  uint64_t operator++() { ++ticks; return ++bar; }

  void display() {
    float progress = (float) ticks / total_ticks;
    int pos = (int) (bar_width * progress);
    if (prevProgress > 50 && prevProgress == pos) {
      return;
    }
    prevProgress = pos;

    print();
    bar.display();
  }

  void done() {
    print();
    bar.done();
  }
};