#ifndef CORIOLIS_GALAXY_H_
#define CORIOLIS_GALAXY_H_

#include <cstdint>
#include <string>
#include <vector>

namespace coriolis {

/// A star system and what is known about it. Everything is worked out from the galaxy number and the system's place
/// in it, so a galaxy is always the same and needs no storage.
struct StarSystem {
  std::string name;
  int galaxy = 0;
  int index = 0;
  /// Position on the galactic chart: x is 0 to 255 and y is 0 to 127
  int x = 0;
  int y = 0;
  /// 0 to 7: rich industrial ... poor agricultural (see EconomyName)
  int economy = 0;
  /// 0 to 7: anarchy ... corporate state (see GovernmentName)
  int government = 0;
  /// 1 to 15
  int tech_level = 1;
  /// In tenths of billions of people
  int population = 1;
  /// In millions of credits
  int productivity = 0;
  /// Of the planet, in kilometres
  int radius_km = 0;
  std::string species;
  /// A number to make this system's market and the like differ from the next
  std::uint32_t seed = 0;
};

const char* EconomyName(int economy);
const char* GovernmentName(int government);

constexpr int kSystemsPerGalaxy = 256;
constexpr int kGalaxyCount = 8;

class Galaxy {
 public:
  /// Galaxy `number` is 0 to 7
  explicit Galaxy(int number);

  int number() const { return number_; }
  const std::vector<StarSystem>& systems() const { return systems_; }
  const StarSystem& system(int index) const { return systems_.at(static_cast<std::size_t>(index)); }

  /// Distance between two systems in light years
  double DistanceLy(int a, int b) const;
  /// Systems within `ly` light years of a system, nearest first, not counting the system itself
  std::vector<int> InRange(int from, double ly) const;
  /// The index of the system with the name (ignoring case), or -1
  int Find(const std::string& name) const;
  /// The system nearest a point on the chart
  int Nearest(int x, int y) const;

 private:
  int number_;
  std::vector<StarSystem> systems_;
};

}  // namespace coriolis

#endif  // CORIOLIS_GALAXY_H_
