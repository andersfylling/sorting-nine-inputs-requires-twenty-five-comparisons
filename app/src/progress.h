#pragma once

#include <progresscpp/ProgressBar.hpp>
#include <string>

class Progress {
protected:
  static constexpr uint8_t width{45};

  std::string info;
  progresscpp::ProgressBar bar;

  int prevProgress{-1};
  const unsigned int total_ticks;
  unsigned int ticks = 0;

  void print() const { std::cout << info << " "; }

public:
  Progress(const std::string& prefix, const std::string& suffix, uint64_t total)
      : bar(total, width), total_ticks(total) {
    const auto w{35};

    // <category> :: <description> [==>...
    info.reserve(w);
    info += prefix;
    for (auto i{info.size()}; i < (w - suffix.size()); ++i) {
      info += ' ';
    }
    info.at(11) = ':';
    info.at(12) = ':';
    info += suffix;
  }

  uint64_t operator++() {
    ++ticks;
    return ++bar;
  }
  uint64_t operator+=(const int64_t increment) {
    ticks += increment;
    for (auto i{0}; i < increment; ++i) {
      ++bar;  // TODO: improve..
    }
    return ticks;
  }

  void display() {
    float progress = (float)ticks / total_ticks;
    int pos = (int)((int)width * progress);
    if (prevProgress > 50 && prevProgress == pos) {
      return;
    }
    prevProgress = pos;

    print();
    bar.display();
  }

  void done() {
    for (int64_t i{total_ticks - ticks}; i >= 0; --i) {
      ++bar;
    }
    print();
    bar.done();
  }
};