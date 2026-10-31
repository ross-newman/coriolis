#ifndef OPEN_ELITE_MARKET_H_
#define OPEN_ELITE_MARKET_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "open_elite/galaxy.h"

namespace open_elite {

enum class Commodity {
  kFood,
  kTextiles,
  kRadioactives,
  kSlaves,
  kLiquor,
  kLuxuries,
  kNarcotics,
  kComputers,
  kMachinery,
  kAlloys,
  kFirearms,
  kFurs,
  kMinerals,
  kGold,
  kPlatinum,
  kGemStones,
  kAlienItems,
  kCount
};

constexpr std::size_t kCommodityCount = static_cast<std::size_t>(Commodity::kCount);

enum class Unit { kTonnes, kKilograms, kGrams };

struct CommodityInfo {
  const char* name;
  Unit unit;
  /// The price before the system's economy and the day's mood change it, in tenths of a credit
  int base_price;
  /// How much each step of economy (0 industrial ... 7 agricultural) moves the price
  int economy_factor;
  int base_quantity;
  int quantity_factor;
  /// How much the day's mood moves the price and the quantity
  int mood_mask;
  /// Smuggling: the police take a dim view
  bool illegal;
};

const CommodityInfo& CommodityData(Commodity c);
const char* UnitName(Unit u);

struct MarketEntry {
  /// Price of one unit in tenths of a credit
  int price = 0;
  /// Units on sale
  int quantity = 0;
};

/// The goods for sale at one station. `mood` changes now and then, so prices drift between visits.
class Market {
 public:
  static Market ForSystem(const StarSystem& system, std::uint32_t mood);

  const MarketEntry& entry(Commodity c) const { return entries_[static_cast<std::size_t>(c)]; }
  MarketEntry& entry(Commodity c) { return entries_[static_cast<std::size_t>(c)]; }

 private:
  std::array<MarketEntry, kCommodityCount> entries_{};
};

}  // namespace open_elite

#endif  // OPEN_ELITE_MARKET_H_
