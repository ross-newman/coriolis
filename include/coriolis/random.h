#ifndef CORIOLIS_RANDOM_H_
#define CORIOLIS_RANDOM_H_

#include <cstdint>

namespace coriolis {

/// A small, fast, repeatable random number generator (splitmix64). The same seed always gives the same numbers, so a
/// game can be replayed and tests can rely on it.
class Random {
 public:
  explicit Random(std::uint64_t seed = 1) : state_(seed) {}

  std::uint64_t Next() {
    state_ += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state_;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }

  /// 0 up to but not including 1
  double Uniform() { return static_cast<double>(Next() >> 11) / 9007199254740992.0; }

  double Range(double low, double high) { return low + (high - low) * Uniform(); }

  /// A whole number from 0 up to but not including n (0 if n is 0)
  std::uint32_t Below(std::uint32_t n) { return n == 0 ? 0 : static_cast<std::uint32_t>(Next() % n); }

  /// True with the given probability
  bool Chance(double probability) { return Uniform() < probability; }

  std::uint64_t state() const { return state_; }

 private:
  std::uint64_t state_;
};

}  // namespace coriolis

#endif  // CORIOLIS_RANDOM_H_
