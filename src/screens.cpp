#include "coriolis/screens.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "coriolis/blueprint.h"
#include "coriolis/commodity_models.h"
#include "coriolis/equipment_models.h"
#include "coriolis/missions.h"
#include "coriolis/renderer.h"

namespace coriolis {

namespace {

const Rgb kText{170, 230, 190};
const Rgb kTitle{255, 230, 120};
const Rgb kDim{100, 130, 115};
const Rgb kBright{255, 255, 255};

struct Page {
  Image& image;
  std::uint32_t scale;
  std::int64_t line_height;
  std::int64_t margin;
  std::int64_t y;

  explicit Page(Image& img) : image(img) {
    img.Clear({4, 8, 6});
    scale = std::max<std::uint32_t>(1, img.height() / 200);
    line_height = static_cast<std::int64_t>(10 * scale);
    margin = static_cast<std::int64_t>(8 * scale);
    y = margin;
  }

  void Title(const std::string& text) {
    const std::int64_t w = static_cast<std::int64_t>(text.size()) * 6 * scale;
    DrawText(image, text, (static_cast<std::int64_t>(image.width()) - w) / 2, y, scale, kTitle);
    y += line_height;
    for (std::int64_t x = margin; x < static_cast<std::int64_t>(image.width()) - margin; ++x) {
      image.Set(x, y, {60, 110, 80});
    }
    y += line_height / 2;
  }

  void Line(const std::string& text, Rgb colour = kText, std::int64_t column = 0) {
    DrawText(image, text, margin + column * 6 * static_cast<std::int64_t>(scale), y, scale, colour);
    y += line_height;
  }

  /// Text at a column without moving down
  void At(const std::string& text, std::int64_t column, Rgb colour = kText) {
    DrawText(image, text, margin + column * 6 * static_cast<std::int64_t>(scale), y, scale, colour);
  }

  void Next() { y += line_height; }
  void Gap() { y += line_height / 2; }
};

std::string Credits(std::int64_t tenths) {
  char buffer[48];
  std::snprintf(buffer, sizeof buffer, "%lld.%lld CR", static_cast<long long>(tenths / 10), static_cast<long long>(tenths % 10));
  return buffer;
}

/// Draw a model in a rectangle of the screen, a little larger than a ship is drawn. Nothing if the rectangle is tiny.
void DrawPreview(Image& image, const Model& model, std::int64_t x0, std::int64_t y0, std::int64_t w, std::int64_t h,
                 float angle) {
  if (w < 24 || h < 24) {
    return;
  }
  const auto tile_w = static_cast<std::uint32_t>(w);
  const auto tile_h = static_cast<std::uint32_t>(h);
  Image tile(tile_w, tile_h);
  tile.Clear({4, 8, 6});
  RenderOptions zoomed;
  zoomed.camera_distance = 8.0f;
  Renderer(zoomed).DrawShip(tile, model, angle);
  for (std::uint32_t y = 0; y < tile_h; ++y) {
    for (std::uint32_t x = 0; x < tile_w; ++x) {
      const std::uint8_t* px = tile.data() + (static_cast<std::size_t>(y) * tile_w + x) * 3;
      image.Set(x0 + x, y0 + y, {px[0], px[1], px[2]});
    }
  }
}

struct EquipmentRow {
  Item item;
  const char* name;
};

const EquipmentRow kEquipment[] = {
    {Item::kFuel, "FUEL"},
    {Item::kMissile, "MISSILE"},
    {Item::kEcm, "E.C.M. SYSTEM"},
    {Item::kEscapePod, "ESCAPE POD"},
    {Item::kFuelScoop, "FUEL SCOOPS"},
    {Item::kEnergyBomb, "ENERGY BOMB"},
    {Item::kBeamLaser, "BEAM LASER"},
    {Item::kDockingComputer, "DOCKING COMPUTER"},
    {Item::kMilitaryLaser, "MILITARY LASER"},
    {Item::kCargoBay, "EXTRA CARGO BAY"},
    {Item::kGalacticHyperdrive, "GALACTIC HYPERDRIVE"},
};

bool Owned(const Commander& c, Item item) {
  switch (item) {
    case Item::kEcm:
      return c.equipment.ecm;
    case Item::kFuelScoop:
      return c.equipment.fuel_scoop;
    case Item::kDockingComputer:
      return c.equipment.docking_computer;
    case Item::kEnergyBomb:
      return c.equipment.energy_bomb;
    case Item::kEscapePod:
      return c.equipment.escape_pod;
    case Item::kBeamLaser:
      return c.equipment.laser_level >= 1;
    case Item::kMilitaryLaser:
      return c.equipment.laser_level >= 2;
    case Item::kMissile:
      return c.missiles >= 4;
    case Item::kCargoBay:
      return c.equipment.cargo_bay;
    case Item::kGalacticHyperdrive:
      return c.equipment.galactic_hyperdrive;
    case Item::kFuel:
      return c.fuel >= kMaxFuel - 1e-6;
  }
  return false;
}

}  // namespace

int EquipmentCount() { return static_cast<int>(sizeof(kEquipment) / sizeof(kEquipment[0])); }

Item EquipmentItem(int row) { return kEquipment[std::clamp(row, 0, EquipmentCount() - 1)].item; }

void DrawStatusScreen(Image& image, const Game& game) {
  Page p(image);
  const Commander& c = game.commander();
  p.Title("COMMANDER " + c.name);
  p.Line("PRESENT SYSTEM:      " + game.system().name);
  const int dest = game.selected_system();
  p.Line("HYPERSPACE SYSTEM:   " + (dest >= 0 ? game.galaxy().system(dest).name : std::string("NONE")));
  const char* condition = game.mode() == GameMode::kDocked ? "DOCKED" : (game.mode() == GameMode::kDead ? "DEAD" : "IN FLIGHT");
  p.Line(std::string("CONDITION:          ") + condition);
  char buffer[64];
  std::snprintf(buffer, sizeof buffer, "FUEL:               %.1f LIGHT YEARS", c.fuel);
  p.Line(buffer);
  p.Line("CASH:               " + Credits(c.credits));
  if (c.mission.kind != MissionKind::kNone) {
    p.Line("MISSION:            " + MissionStatus(c.mission, game.galaxy()), kBright);
  }
  p.Line(std::string("LEGAL STATUS:       ") + LegalName(c.legal_status), c.legal_status > 0 ? Rgb{255, 140, 90} : kText);
  std::snprintf(buffer, sizeof buffer, "RATING:             %s (%.0f KILLS)", RankName(c.kills), std::floor(c.kills));
  p.Line(buffer, kBright);
  p.Gap();
  p.Line("EQUIPMENT:", kTitle);
  std::snprintf(buffer, sizeof buffer, "  MISSILES: %d", c.missiles);
  p.Line(buffer);
  const struct {
    bool have;
    const char* name;
  } fitted[] = {{c.equipment.ecm, "E.C.M. SYSTEM"},          {c.equipment.fuel_scoop, "FUEL SCOOPS"},
                {c.equipment.docking_computer, "DOCKING COMPUTER"}, {c.equipment.energy_bomb, "ENERGY BOMB"},
                {c.equipment.escape_pod, "ESCAPE POD"},
                {c.equipment.cargo_bay, "EXTRA CARGO BAY"},
                {c.equipment.galactic_hyperdrive, "GALACTIC HYPERDRIVE"}};
  for (const auto& f : fitted) {
    if (f.have) {
      p.Line(std::string("  ") + f.name);
    }
  }
  const char* lasers[] = {"PULSE LASER", "BEAM LASER", "MILITARY LASER"};
  p.Line(std::string("  ") + lasers[std::clamp(c.equipment.laser_level, 0, 2)]);
  p.Gap();
  std::snprintf(buffer, sizeof buffer, "CARGO: %d OF %d", CargoUsed(c), c.cargo_capacity);
  p.Line(buffer, kTitle);
  bool any = false;
  for (std::size_t i = 0; i < kCommodityCount; ++i) {
    if (c.cargo[i] > 0) {
      const CommodityInfo& info = CommodityData(static_cast<Commodity>(i));
      std::snprintf(buffer, sizeof buffer, "  %d%s %s", c.cargo[i], UnitName(info.unit), info.name);
      p.Line(buffer);
      any = true;
    }
  }
  if (!any) {
    p.Line("  EMPTY", kDim);
  }
}

void DrawMarketScreen(Image& image, const Game& game, int selected, float angle) {
  Page p(image);
  const Commander& c = game.commander();
  p.Title("MARKET PRICES AT " + game.system().name);
  p.At("PRODUCT", 2, kTitle);
  p.At("PRICE", 22, kTitle);
  p.At("FOR SALE", 32, kTitle);
  p.At("IN HOLD", 44, kTitle);
  p.Next();
  const std::int64_t list_top = p.y;
  for (std::size_t i = 0; i < kCommodityCount; ++i) {
    const Commodity item = static_cast<Commodity>(i);
    const CommodityInfo& info = CommodityData(item);
    const MarketEntry& e = game.market().entry(item);
    const bool here = static_cast<int>(i) == selected;
    const Rgb colour = here ? kBright : kText;
    char price[32];
    std::snprintf(price, sizeof price, "%d.%d", e.price / 10, e.price % 10);
    char sale[32];
    std::snprintf(sale, sizeof sale, "%d%s", e.quantity, UnitName(info.unit));
    char hold[32];
    std::snprintf(hold, sizeof hold, "%d", c.cargo[i]);
    p.At(here ? ">" : " ", 0, kTitle);
    p.At(info.name, 2, colour);
    p.At(price, 22, colour);
    p.At(sale, 32, colour);
    p.At(hold, 44, c.cargo[i] > 0 ? kTitle : kDim);
    p.Next();
  }
  p.Gap();
  char line[96];
  std::snprintf(line, sizeof line, "CASH: %s   HOLD: %d OF %d", Credits(c.credits).c_str(), CargoUsed(c), c.cargo_capacity);
  p.Line(line, kBright);

  // The highlighted good in 3D, to the right of the columns, when the picture is wide enough to have room
  if (selected >= 0 && selected < static_cast<int>(kCommodityCount)) {
    const std::int64_t x0 = p.margin + 52 * 6 * static_cast<std::int64_t>(p.scale);
    const std::int64_t room_w = static_cast<std::int64_t>(image.width()) - x0 - p.margin;
    const std::int64_t room_h = static_cast<std::int64_t>(image.height()) - list_top - p.margin;
    DrawPreview(image, GetCommodityModel(static_cast<Commodity>(selected)), x0, list_top, room_w,
                std::min(room_w, room_h), angle);
  }
}

void DrawEquipmentScreen(Image& image, const Game& game, int selected, float angle) {
  Page p(image);
  const Commander& c = game.commander();
  p.Title("EQUIP SHIP");
  char line[96];
  std::snprintf(line, sizeof line, "CASH: %s   TECH LEVEL %d", Credits(c.credits).c_str(), game.system().tech_level);
  p.Line(line, kBright);
  p.Gap();
  for (int i = 0; i < EquipmentCount(); ++i) {
    const EquipmentRow& row = kEquipment[i];
    const bool here = i == selected;
    const bool for_sale = game.system().tech_level >= ItemTechLevel(row.item);
    const bool owned = Owned(c, row.item);
    Rgb colour = here ? kBright : kText;
    if (!for_sale) {
      colour = kDim;
    }
    char price[48];
    if (!for_sale) {
      std::snprintf(price, sizeof price, "NOT SOLD HERE");
    } else if (owned) {
      std::snprintf(price, sizeof price, row.item == Item::kFuel ? "FULL" : "FITTED");
    } else {
      std::snprintf(price, sizeof price, "%s", Credits(ItemPrice(row.item)).c_str());
    }
    p.At(here ? ">" : " ", 0, kTitle);
    p.At(row.name, 2, colour);
    p.At(price, 26, colour);
    p.Next();
  }

  // The highlighted item in 3D, in the space under the list
  if (selected >= 0 && selected < EquipmentCount()) {
    const std::int64_t top = p.y + p.line_height / 2;
    const std::int64_t room_h = static_cast<std::int64_t>(image.height()) - top - p.margin;
    const std::int64_t room_w = static_cast<std::int64_t>(image.width()) - 2 * p.margin;
    const std::int64_t side = std::min(room_h, room_w / 2);
    const std::int64_t tile_w = std::min(room_w, side * 2);
    DrawPreview(image, GetItemModel(kEquipment[selected].item), (static_cast<std::int64_t>(image.width()) - tile_w) / 2, top,
                tile_w, side, angle);
  }
}

void DrawMissionsScreen(Image& image, const Game& game) {
  Page p(image);
  const Commander& c = game.commander();
  p.Title("MISSIONS");
  char line[96];
  std::snprintf(line, sizeof line, "JOBS DONE: %d OF %d", std::min(c.story_step, kStorySteps), kStorySteps);
  p.Line(line, kDim);
  p.Gap();
  Mission offer;
  if (c.mission.kind != MissionKind::kNone) {
    p.Line("UNDER WAY: " + MissionTitle(c.mission), kTitle);
    p.Gap();
    for (const std::string& text : MissionBriefing(c.mission, game.galaxy())) {
      p.Line(text);
    }
    p.Gap();
    p.Line("TO DO: " + MissionStatus(c.mission, game.galaxy()), kBright);
    p.Gap();
    p.Line("BACKSPACE GIVES UP THE JOB", kDim);
  } else if (game.MissionOnOffer(&offer)) {
    p.Line("ON OFFER: " + MissionTitle(offer), kTitle);
    p.Gap();
    for (const std::string& text : MissionBriefing(offer, game.galaxy())) {
      p.Line(text);
    }
    p.Gap();
    p.Line("ENTER TAKES THE JOB", kBright);
  } else if (c.story_step >= kStorySteps) {
    p.Line("NO MORE WORK FROM THE WOMAN AT THE BAR", kTitle);
    p.Gap();
    p.Line("THE PACKETS ARE DELIVERED AND THE FLEET IS BROKEN. THE NEXT GALAXY");
    p.Line("IS ONE JUMP AWAY. TRADE, HUNT AND EXPLORE.");
  } else {
    p.Line("NO WORK ON OFFER HERE", kTitle);
    p.Gap();
    p.Line("THIS SYSTEM HAS NO NEIGHBOURS WITHIN REACH OF THE FUEL");
  }
}

void DrawShipyardScreen(Image& image, const Game& game, int selected, float angle) {
  Page p(image);
  const Commander& c = game.commander();
  p.Title("SHIPYARD");
  const ShipForSale* mine = FindShipForSale(c.ship);
  char line[112];
  std::snprintf(line, sizeof line, "CASH: %s   TECH LEVEL %d", Credits(c.credits).c_str(), game.system().tech_level);
  p.Line(line, kBright);
  std::snprintf(line, sizeof line, "YOUR SHIP: %s   TRADE-IN %s", mine != nullptr ? mine->name : "UNKNOWN",
                Credits(TradeInValue(c)).c_str());
  p.Line(line);
  p.Gap();
  p.At("SHIP", 2, kDim);
  p.At("HOLD", 20, kDim);
  p.At("PRICE", 28, kDim);
  p.Next();
  for (int i = 0; i < ShipForSaleCount(); ++i) {
    const ShipForSale& s = ShipForSaleAt(i);
    const bool here = i == selected;
    const bool mine_now = c.ship == s.id;
    const bool for_sale = game.system().tech_level >= s.tech_level;
    const Rgb colour = !for_sale ? kDim : (here ? kBright : kText);
    const int hold = s.hold + (c.equipment.cargo_bay ? kCargoBayExtra : 0);
    char tonnes[16];
    std::snprintf(tonnes, sizeof tonnes, "%d T", hold);
    char price[48];
    if (mine_now) {
      std::snprintf(price, sizeof price, "YOURS");
    } else if (!for_sale) {
      std::snprintf(price, sizeof price, "NOT SOLD HERE");
    } else {
      std::snprintf(price, sizeof price, "%s", Credits(static_cast<std::int64_t>(s.price) - TradeInValue(c)).c_str());
    }
    p.At(here ? ">" : " ", 0, kTitle);
    p.At(s.name, 2, colour);
    p.At(tonnes, 20, colour);
    p.At(price, 28, colour);
    p.Next();
  }
  p.Gap();
  p.Line("THE PRICE IS WHAT YOU PAY AFTER THE TRADE-IN OF YOUR SHIP", kDim);
  p.Line("EQUIPMENT AND MISSILES GO ACROSS; THE HOLD MUST BE ABLE TO TAKE YOUR CARGO", kDim);

  if (selected >= 0 && selected < ShipForSaleCount()) {
    const Blueprint* bp = FindBuiltInBlueprint(ShipForSaleAt(selected).id);
    if (bp != nullptr) {
      const std::int64_t top = p.y + p.line_height / 2;
      const std::int64_t room_h = static_cast<std::int64_t>(image.height()) - top - p.margin;
      const std::int64_t room_w = static_cast<std::int64_t>(image.width()) - 2 * p.margin;
      const std::int64_t side = std::min(room_h, room_w / 2);
      const std::int64_t tile_w = std::min(room_w, side * 2);
      DrawPreview(image, BlueprintToModel(*bp), (static_cast<std::int64_t>(image.width()) - tile_w) / 2, top, tile_w, side,
                  angle);
    }
  }
}

void DrawChartScreen(Image& image, const Game& game) {
  Page p(image);
  const Galaxy& galaxy = game.galaxy();
  char title[64];
  std::snprintf(title, sizeof title, "GALACTIC CHART %d", galaxy.number() + 1);
  p.Title(title);

  const std::int64_t w = static_cast<std::int64_t>(image.width());
  const std::int64_t area_top = p.y;
  const std::int64_t area_h = static_cast<std::int64_t>(image.height()) - area_top - 4 * p.line_height;
  // One chart unit is the same distance across and down
  const double unit = std::min(static_cast<double>(w - 2 * p.margin) / 256.0, static_cast<double>(area_h) / 128.0);
  const double ox = static_cast<double>(p.margin) + (static_cast<double>(w - 2 * p.margin) - 256.0 * unit) / 2.0;
  const double oy = static_cast<double>(area_top);
  const auto sx = [&](const StarSystem& s) { return ox + s.x * unit; };
  const auto sy = [&](const StarSystem& s) { return oy + s.y * unit; };

  const StarSystem& here = game.system();
  // The distance the fuel will carry the ship, in chart units (a light year is four)
  const double reach = game.commander().fuel * 4.0 * unit;
  for (int i = 0; i < 360; ++i) {
    const double a = 2.0 * 3.141592653589793 * i / 360.0;
    image.Set(static_cast<std::int64_t>(sx(here) + reach * std::cos(a)), static_cast<std::int64_t>(sy(here) + reach * std::sin(a)),
              {70, 70, 150});
  }
  for (const StarSystem& s : galaxy.systems()) {
    const std::int64_t x = static_cast<std::int64_t>(sx(s));
    const std::int64_t y = static_cast<std::int64_t>(sy(s));
    const bool in_range = galaxy.DistanceLy(here.index, s.index) <= game.commander().fuel;
    const Rgb c = in_range ? Rgb{255, 240, 160} : Rgb{150, 150, 130};
    image.Set(x, y, c);
    image.Set(x + 1, y, c);
    if (p.scale > 1) {
      image.Set(x, y + 1, c);
      image.Set(x + 1, y + 1, c);
    }
    if (in_range && s.index != here.index && galaxy.DistanceLy(here.index, s.index) <= 4.0) {
      DrawText(image, s.name, x + 3, y - 3, 1, {120, 150, 130});
    }
  }
  // Where we are, and where we are going
  const std::int64_t hx = static_cast<std::int64_t>(sx(here));
  const std::int64_t hy = static_cast<std::int64_t>(sy(here));
  const std::int64_t arm = 3 * static_cast<std::int64_t>(p.scale);
  for (std::int64_t k = -arm; k <= arm; ++k) {
    image.Set(hx + k, hy, {255, 60, 60});
    image.Set(hx, hy + k, {255, 60, 60});
  }
  const int dest = game.selected_system();
  if (dest >= 0) {
    const StarSystem& d = galaxy.system(dest);
    const double r = 4.0 * p.scale;
    for (int i = 0; i < 90; ++i) {
      const double a = 2.0 * 3.141592653589793 * i / 90.0;
      image.Set(static_cast<std::int64_t>(sx(d) + r * std::cos(a)), static_cast<std::int64_t>(sy(d) + r * std::sin(a)), {80, 255, 120});
    }
  }
  p.y = static_cast<std::int64_t>(image.height()) - 3 * p.line_height;
  char line[128];
  if (dest >= 0) {
    std::snprintf(line, sizeof line, "%s  %.1f LY  %s  %s", galaxy.system(dest).name.c_str(),
                  galaxy.DistanceLy(here.index, dest), EconomyName(galaxy.system(dest).economy),
                  GovernmentName(galaxy.system(dest).government));
  } else {
    std::snprintf(line, sizeof line, "YOU ARE AT %s. NO DESTINATION CHOSEN.", here.name.c_str());
  }
  p.Line(line, kBright);
  std::snprintf(line, sizeof line, "FUEL %.1f LY", game.commander().fuel);
  p.Line(line, kText);
}

void DrawSystemScreen(Image& image, const Game& game, int system_index) {
  Page p(image);
  const Galaxy& galaxy = game.galaxy();
  const StarSystem& s = galaxy.system(std::clamp(system_index, 0, kSystemsPerGalaxy - 1));
  p.Title("DATA ON " + s.name);
  char line[128];
  std::snprintf(line, sizeof line, "DISTANCE:       %.1f LIGHT YEARS", galaxy.DistanceLy(game.commander().system, s.index));
  p.Line(line);
  p.Line(std::string("ECONOMY:        ") + EconomyName(s.economy));
  p.Line(std::string("GOVERNMENT:     ") + GovernmentName(s.government));
  std::snprintf(line, sizeof line, "TECH LEVEL:     %d", s.tech_level);
  p.Line(line);
  std::snprintf(line, sizeof line, "POPULATION:     %d.%d BILLION", s.population / 10, s.population % 10);
  p.Line(line);
  p.Line("SPECIES:        " + s.species);
  std::snprintf(line, sizeof line, "GROSS PRODUCT:  %d M CR", s.productivity);
  p.Line(line);
  std::snprintf(line, sizeof line, "AVERAGE RADIUS: %d KM", s.radius_km);
  p.Line(line);
}

}  // namespace coriolis
