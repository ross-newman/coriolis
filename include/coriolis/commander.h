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
  /// Adds `kCargoBayExtra` tonnes to the hold
  bool cargo_bay = false;
  /// Jumps to the next galaxy, once
  bool galactic_hyperdrive = false;
  /// 0 pulse laser, 1 beam laser, 2 military laser
  int laser_level = 0;
};

/// A job the pilot has taken: carry a packet to a system, or shoot down pirates. See missions.h.
enum class MissionKind { kNone, kDeliver, kHunt };

struct Mission {
  MissionKind kind = MissionKind::kNone;
  /// Which step of the story it is
  int step = 0;
  /// The system to deliver to, for a delivery
  int destination = -1;
  /// How many pirates to shoot down, and how many have been
  int count = 0;
  int progress = 0;
  /// In tenths of a credit; 0 for the last step, which is paid in a hyperdrive
  int reward = 0;
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
  /// The job under way, and the next step of the story to be offered (see missions.h)
  Mission mission;
  int story_step = 0;
};

constexpr double kMaxFuel = 7.0;
/// Tonnes the extra cargo bay adds to the hold
constexpr int kCargoBayExtra = 15;

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
  kFuel,
  kCargoBay,
  kGalacticHyperdrive
};

/// What an item costs in tenths of a credit (for fuel, per light year), and the tech level a system needs to sell it
int ItemPrice(Item item);
int ItemTechLevel(Item item);
const char* ItemName(Item item);
/// Buy an item (fuel: fill the tank as far as money allows). False if it is not for sale here, is already owned or
/// cannot be afforded.
bool BuyItem(Commander& c, Item item, int system_tech_level);

/// A ship the shipyard sells. `hold` is its cargo space in tonnes, `price` is in tenths of a credit.
struct ShipForSale {
  const char* id;
  const char* name;
  int price;
  int hold;
  int tech_level;
};
int ShipForSaleCount();
const ShipForSale& ShipForSaleAt(int index);
/// The entry for a ship id, or nullptr if the shipyard does not deal in it
const ShipForSale* FindShipForSale(const std::string& id);
/// What the yard gives for the commander's ship in part exchange, in tenths of a credit
int TradeInValue(const Commander& c);
/// Swap the commander's ship for one in the yard, paying the price less the trade-in. Equipment and missiles go across;
/// the hold becomes the new ship's (plus the extra bay if fitted). False, changing nothing, if the yard is not at this
/// tech level, it is the ship already flown, the money is short or the cargo would not fit.
bool BuyShip(Commander& c, int index, int system_tech_level);

Json CommanderToJson(const Commander& c);
bool CommanderFromJson(const Json& json, Commander* out, std::string* error);
bool SaveCommander(const Commander& c, const std::string& path, std::string* error);
bool LoadCommander(const std::string& path, Commander* out, std::string* error);

}  // namespace coriolis

#endif  // CORIOLIS_COMMANDER_H_
