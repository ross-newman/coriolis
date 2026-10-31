#include "open_elite/commander.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace open_elite {

int CargoUsed(const Commander& c) {
  int used = 0;
  for (const int n : c.cargo) {
    used += n;
  }
  return used;
}

int CargoFree(const Commander& c) { return std::max(0, c.cargo_capacity - CargoUsed(c)); }

const char* RankName(double kills) {
  struct Rank {
    double from;
    const char* name;
  };
  static const Rank kRanks[] = {{0, "Harmless"},    {8, "Mostly Harmless"}, {16, "Poor"},
                                {32, "Average"},    {64, "Above Average"},  {128, "Competent"},
                                {512, "Dangerous"}, {2560, "Deadly"},       {6400, "Elite"}};
  const char* name = kRanks[0].name;
  for (const Rank& r : kRanks) {
    if (kills >= r.from) {
      name = r.name;
    }
  }
  return name;
}

const char* LegalName(int legal_status) {
  if (legal_status <= 0) {
    return "Clean";
  }
  return legal_status < 50 ? "Offender" : "Fugitive";
}

bool BuyCargo(Commander& c, Market& market, Commodity item, int quantity) {
  MarketEntry& e = market.entry(item);
  if (quantity <= 0 || e.quantity < quantity || CargoFree(c) < quantity) {
    return false;
  }
  const std::int64_t cost = static_cast<std::int64_t>(e.price) * quantity;
  if (cost > c.credits) {
    return false;
  }
  c.credits -= cost;
  c.cargo[static_cast<std::size_t>(item)] += quantity;
  e.quantity -= quantity;
  return true;
}

bool SellCargo(Commander& c, Market& market, Commodity item, int quantity) {
  int& held = c.cargo[static_cast<std::size_t>(item)];
  if (quantity <= 0 || held < quantity) {
    return false;
  }
  MarketEntry& e = market.entry(item);
  c.credits += static_cast<std::int64_t>(e.price) * quantity;
  held -= quantity;
  e.quantity += quantity;
  return true;
}

int ItemPrice(Item item) {
  switch (item) {
    case Item::kMissile:
      return 300;
    case Item::kEcm:
      return 6000;
    case Item::kFuelScoop:
      return 5250;
    case Item::kDockingComputer:
      return 15000;
    case Item::kEnergyBomb:
      return 9000;
    case Item::kEscapePod:
      return 10000;
    case Item::kBeamLaser:
      return 10000;
    case Item::kMilitaryLaser:
      return 60000;
    case Item::kFuel:
      return 20;
  }
  return 0;
}

int ItemTechLevel(Item item) {
  switch (item) {
    case Item::kMissile:
    case Item::kFuel:
      return 1;
    case Item::kEcm:
    case Item::kEscapePod:
      return 3;
    case Item::kFuelScoop:
      return 5;
    case Item::kEnergyBomb:
    case Item::kBeamLaser:
      return 7;
    case Item::kDockingComputer:
      return 9;
    case Item::kMilitaryLaser:
      return 10;
  }
  return 1;
}

const char* ItemName(Item item) {
  switch (item) {
    case Item::kMissile:
      return "Missile";
    case Item::kEcm:
      return "ECM";
    case Item::kFuelScoop:
      return "Fuel scoop";
    case Item::kDockingComputer:
      return "Docking computer";
    case Item::kEnergyBomb:
      return "Energy bomb";
    case Item::kEscapePod:
      return "Escape pod";
    case Item::kBeamLaser:
      return "Beam laser";
    case Item::kMilitaryLaser:
      return "Military laser";
    case Item::kFuel:
      return "Fuel";
  }
  return "";
}

bool BuyItem(Commander& c, Item item, int system_tech_level) {
  if (system_tech_level < ItemTechLevel(item)) {
    return false;
  }
  const std::int64_t price = ItemPrice(item);
  const auto pay = [&c, price]() {
    if (c.credits < price) {
      return false;
    }
    c.credits -= price;
    return true;
  };
  switch (item) {
    case Item::kMissile:
      if (c.missiles >= 4) {
        return false;
      }
      if (!pay()) {
        return false;
      }
      ++c.missiles;
      return true;
    case Item::kEcm:
      if (c.equipment.ecm || !pay()) {
        return false;
      }
      c.equipment.ecm = true;
      return true;
    case Item::kFuelScoop:
      if (c.equipment.fuel_scoop || !pay()) {
        return false;
      }
      c.equipment.fuel_scoop = true;
      return true;
    case Item::kDockingComputer:
      if (c.equipment.docking_computer || !pay()) {
        return false;
      }
      c.equipment.docking_computer = true;
      return true;
    case Item::kEnergyBomb:
      if (c.equipment.energy_bomb || !pay()) {
        return false;
      }
      c.equipment.energy_bomb = true;
      return true;
    case Item::kEscapePod:
      if (c.equipment.escape_pod || !pay()) {
        return false;
      }
      c.equipment.escape_pod = true;
      return true;
    case Item::kBeamLaser:
      if (c.equipment.laser_level >= 1 || !pay()) {
        return false;
      }
      c.equipment.laser_level = 1;
      return true;
    case Item::kMilitaryLaser:
      if (c.equipment.laser_level >= 2 || !pay()) {
        return false;
      }
      c.equipment.laser_level = 2;
      return true;
    case Item::kFuel: {
      // Fill the tank in tenths of a light year, as far as the money goes
      const int tenths = static_cast<int>(std::floor((kMaxFuel - c.fuel) * 10.0 + 1e-9));
      const int affordable =
          static_cast<int>(std::min<std::int64_t>(tenths, c.credits / std::max<std::int64_t>(1, price / 10)));
      if (affordable <= 0) {
        return false;
      }
      c.credits -= static_cast<std::int64_t>(affordable) * (price / 10);
      c.fuel = std::min(kMaxFuel, c.fuel + affordable / 10.0);
      return true;
    }
  }
  return false;
}

Json CommanderToJson(const Commander& c) {
  Json root = Json::MakeObject();
  root.Set("format", Json::String("open-elite-commander"));
  root.Set("version", Json::Number(1));
  root.Set("name", Json::String(c.name));
  root.Set("credits", Json::Number(static_cast<double>(c.credits)));
  root.Set("fuel", Json::Number(c.fuel));
  root.Set("galaxy", Json::Number(c.galaxy));
  root.Set("system", Json::Number(c.system));
  Json cargo = Json::MakeArray();
  for (const int n : c.cargo) {
    cargo.Push(Json::Number(n));
  }
  root.Set("cargo", std::move(cargo));
  root.Set("cargo_capacity", Json::Number(c.cargo_capacity));
  root.Set("missiles", Json::Number(c.missiles));
  Json eq = Json::MakeObject();
  eq.Set("ecm", Json::Bool(c.equipment.ecm));
  eq.Set("fuel_scoop", Json::Bool(c.equipment.fuel_scoop));
  eq.Set("docking_computer", Json::Bool(c.equipment.docking_computer));
  eq.Set("energy_bomb", Json::Bool(c.equipment.energy_bomb));
  eq.Set("escape_pod", Json::Bool(c.equipment.escape_pod));
  eq.Set("laser_level", Json::Number(c.equipment.laser_level));
  root.Set("equipment", std::move(eq));
  root.Set("legal_status", Json::Number(c.legal_status));
  root.Set("kills", Json::Number(c.kills));
  root.Set("market_mood", Json::Number(static_cast<double>(c.market_mood)));
  root.Set("ship", Json::String(c.ship));
  return root;
}

namespace {

bool Fail(std::string* error, const std::string& message) {
  if (error != nullptr) {
    *error = message;
  }
  return false;
}

bool GetNumber(const Json& j, const char* key, double* out, std::string* error) {
  const Json* m = j.Find(key);
  if (m == nullptr || !m->is_number()) {
    return Fail(error, std::string("commander: \"") + key + "\" is missing or not a number");
  }
  *out = m->number_value();
  return true;
}

bool GetFlag(const Json& j, const char* key, bool* out, std::string* error) {
  const Json* m = j.Find(key);
  if (m == nullptr || !m->is_bool()) {
    return Fail(error, std::string("commander: equipment \"") + key + "\" is missing or not true/false");
  }
  *out = m->bool_value();
  return true;
}

}  // namespace

bool CommanderFromJson(const Json& json, Commander* out, std::string* error) {
  const Json* format = json.Find("format");
  if (!json.is_object() || format == nullptr || !format->is_string() ||
      format->string_value() != "open-elite-commander") {
    return Fail(error, "commander: not an open-elite-commander file");
  }
  Commander c;
  double v = 0.0;
  const Json* name = json.Find("name");
  const Json* ship = json.Find("ship");
  const Json* cargo = json.Find("cargo");
  const Json* eq = json.Find("equipment");
  if (name == nullptr || !name->is_string() || ship == nullptr || !ship->is_string() || cargo == nullptr ||
      !cargo->is_array() || cargo->items().size() != kCommodityCount || eq == nullptr || !eq->is_object()) {
    return Fail(error, "commander: name, ship, cargo or equipment is missing or the wrong kind");
  }
  c.name = name->string_value();
  c.ship = ship->string_value();
  for (std::size_t i = 0; i < kCommodityCount; ++i) {
    const Json& n = cargo->items()[i];
    if (!n.is_number() || n.number_value() < 0.0 || n.number_value() > 100000.0) {
      return Fail(error, "commander: bad cargo amount");
    }
    c.cargo[i] = static_cast<int>(n.number_value());
  }
  if (!GetNumber(json, "credits", &v, error)) {
    return false;
  }
  c.credits = static_cast<std::int64_t>(v);
  if (!GetNumber(json, "fuel", &c.fuel, error) || !GetNumber(json, "kills", &c.kills, error)) {
    return false;
  }
  c.fuel = std::clamp(c.fuel, 0.0, kMaxFuel);
  double galaxy = 0.0, system = 0.0, capacity = 0.0, missiles = 0.0, legal = 0.0, mood = 0.0, laser = 0.0;
  if (!GetNumber(json, "galaxy", &galaxy, error) || !GetNumber(json, "system", &system, error) ||
      !GetNumber(json, "cargo_capacity", &capacity, error) || !GetNumber(json, "missiles", &missiles, error) ||
      !GetNumber(json, "legal_status", &legal, error) || !GetNumber(json, "market_mood", &mood, error) ||
      !GetNumber(*eq, "laser_level", &laser, error)) {
    return false;
  }
  if (galaxy < 0 || galaxy >= kGalaxyCount || system < 0 || system >= kSystemsPerGalaxy || capacity < 0 ||
      capacity > 1000 || missiles < 0 || missiles > 16 || legal < 0 || legal > 255 || laser < 0 || laser > 2) {
    return Fail(error, "commander: a value is out of range");
  }
  c.galaxy = static_cast<int>(galaxy);
  c.system = static_cast<int>(system);
  c.cargo_capacity = static_cast<int>(capacity);
  c.missiles = static_cast<int>(missiles);
  c.legal_status = static_cast<int>(legal);
  c.market_mood = static_cast<std::uint32_t>(mood);
  c.equipment.laser_level = static_cast<int>(laser);
  if (!GetFlag(*eq, "ecm", &c.equipment.ecm, error) || !GetFlag(*eq, "fuel_scoop", &c.equipment.fuel_scoop, error) ||
      !GetFlag(*eq, "docking_computer", &c.equipment.docking_computer, error) ||
      !GetFlag(*eq, "energy_bomb", &c.equipment.energy_bomb, error) ||
      !GetFlag(*eq, "escape_pod", &c.equipment.escape_pod, error)) {
    return false;
  }
  *out = c;
  return true;
}

bool SaveCommander(const Commander& c, const std::string& path, std::string* error) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file) {
    return Fail(error, path + ": cannot create the file");
  }
  file << CommanderToJson(c).Dump();
  file.flush();
  return file ? true : Fail(error, path + ": cannot write the file");
}

bool LoadCommander(const std::string& path, Commander* out, std::string* error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return Fail(error, path + ": cannot open the file");
  }
  std::ostringstream text;
  text << file.rdbuf();
  Json json;
  std::string why;
  if (!Json::Parse(text.str(), &json, &why) || !CommanderFromJson(json, out, &why)) {
    return Fail(error, path + ": " + why);
  }
  return true;
}

}  // namespace open_elite
