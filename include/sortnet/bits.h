#pragma once

#include <type_traits>

#include "sortnet/vendors/github.com/kimwalisch/libpopcnt/libpopcnt.h"

#include "z_environment.h"
namespace sortnet {
  namespace bits {

    inline constexpr bool isset(const uint64_t n, const uint8_t pos) {
      return (n & uint64_t(uint64_t(1) << pos)) > 0;
    }

    inline constexpr uint8_t LSB(const uint64_t n) {
#ifdef __linux__
      return (n > 0) ? __builtin_ffsl(n) - 1 : 0;
#else
      for (int i = 0; i < 64; ++i) {
        if (isset(n, i)) {
          return i;
        }
      }
#endif
    }

    template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    inline constexpr int8_t MSB(const T x) {
      static_assert(std::is_same<uint64_t, T>::value || std::is_same<uint32_t, T>::value
                        || std::is_same<uint16_t, T>::value || std::is_same<uint8_t, T>::value,
                    "numeric type must be unsigned and have the size of range <8, 64>");
      // TODO: clang support?
      if constexpr (std::is_same<uint64_t, T>::value) {
        return (x > 0) ? 63 - __builtin_clzll(x) : -1;
      } else if constexpr (std::is_same<uint32_t, T>::value) {
        return (x > 0) ? 31 - __builtin_clzl(x) : -1;
      } else if constexpr (std::is_same<uint16_t, T>::value) {
        return (x > 0) ? 15 - __builtin_clz(x) : -1;
      } else {
        return (x > 0) ? 7 - __builtin_clz(x) : -1;
      }
    }

    template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    inline constexpr int8_t NextMSB(T &x, const int8_t pos) {
      x ^= T(1) << pos;
      return MSB<T>(x);
    }
  }  // namespace bits
}