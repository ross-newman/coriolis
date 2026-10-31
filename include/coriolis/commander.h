#ifndef CORIOLIS_COMMANDER_H_
#define CORIOLIS_COMMANDER_H_

#include <array>
#include <cstdint>
#include <string>

#include "coriolis/json.h"
#include "coriolis/market.h"

namespace coriolis {

struct Equipment {
  bool ecm = false;
  bool fuel_scoop = false;
  bool docking_computer = false;
  bool energy_bomb = false;
  bool escape_pod = false;
  /// 0 pulse laser, 1 beam laser, 2 military laser
  int laser_level = 0;
};

/// The player: money, fuel, cargo, standing with the law and kills. Everything that is saved.
struct Commander {
  std::string name = "Commander";
  /// Money in tenths of a credit
  std::int64_t credits = 1000;
  /// Light years of fuel
  double fuel = 7.0;
  int galaxy = 0;
  int system = 0;
  std::array<int, kCommodityCount> cargo{};
  int cargo_capacity = 20;
  int missiles = 3;
  Equipment equipment;
  /// 0 clean, 1 to 49 offender, 50 and over fugitive
  int legal_status = 0;
  /// Combat rank points. Different ships are worth different amounts.
  double kills = 0.0;
  /// Changes between visits, so market prices drift
  std::uint32_t market_mood = 1;
  /// The id of the ship flown (a blueprint id)
  std::string ship = "cobra_mk3_trader";
};

constexpr double kMaxFuel = 7.0;

int CargoUsed(const Commander& c);
int CargoFree(const Commander& c);

/// Combat rank from the kills: Harmless, Mostly Harmless, Poor, Average, Above Average, Competent, Dangerous, Deadly or
/// Elite
const char* RankName(double kills);
/// Clean, Offender or Fugitive
const char* LegalName(int legal_status);

/// Buy or sell cargo at a station's market. They change the commander and the market, and return false, changing
/// nothing, if it cannot be done.
bool BuyCargo(Commander& c, Market& market, Commodity item, int quantity);
bool SellCargo(Commander& c, Market& market, Commodity item, int quantity);

enum class Item {
  kMissile,
  kEcm,
  kFuelScoop,
  kDockingComputer,
  kEnergyBomb,
  kEscapePod,
  kBeamLaser,
  kMilitaryLaser,
  kFuel
};

/// What an item costs in tenths of a credit (for fuel, per light year), and the tech level a system needs to sell it
int ItemPrice(Item item);
int ItemTechLevel(Item item);
const char* ItemName(Item item);
/// Buy an item (fuel: fill the tank as far as money allows). False if it is not for sale here, is already owned or
/// cannot be afforded.
bool BuyItem(Commander& c, Item item, int system_tech_level);

Json CommanderToJson(const Commander& c);
bool CommanderFromJson(const Json& json, Commander* out, std::string* error);
bool SaveCommander(const Commander& c, const std::string& path, std::string* error);
bool LoadCommander(const std::string& path, Commander* out, std::string* error);

}  // namespace coriolis

#endif  // CORIOLIS_COMMANDER_H_
