#pragma once

#include <mutex>
#include <queue>
#include <vector>

namespace sortnet {
template <typename T> class Pool {
protected:
  std::deque<T*> objects{};
  std::mutex m;

public:
  constexpr Pool() = default;

  ~Pool() {
    for (T* obj : objects) {
      delete obj;
    }
  }

  T* get() {
    const std::lock_guard<std::mutex> lock(m);
    if (objects.empty()) {
      return new T();
    }

    T* obj = objects.front();
    objects.pop_front();
    return obj;
  }

  void put(T* obj) {
    const std::lock_guard<std::mutex> lock(m);
    objects.push_back(obj);
  }
};
}  // namespace sortnet