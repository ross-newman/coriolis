#ifndef OPEN_ELITE_HASH_H_
#define OPEN_ELITE_HASH_H_

#include <cstdint>

namespace open_elite {

/// A fast integer hash, used to place the stars and to roughen the asteroid
inline std::uint32_t Hash32(std::uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

}  // namespace open_elite

#endif  // OPEN_ELITE_HASH_H_
