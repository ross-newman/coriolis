// flight_view: let the computer pilot fly the game and save what the pilot sees as PNG pictures: the cockpit view with
// its dashboard at intervals, and the station screens (status, market, equipment, charts) when docked.
//
//   flight_view [--seed N] [--seconds S] [--every SECONDS] [--size WxH] [--out DIRECTORY] [--view front|rear|left|right]
//                   [--quality 1-4]   (1 is fastest; more draws bigger and shrinks, for smooth edges; default 2)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "coriolis/bot.h"
#include "coriolis/cockpit.h"
#include "coriolis/game.h"
#include "coriolis/png.h"
#include "coriolis/screens.h"

using namespace coriolis;

namespace {

bool ParseSize(const char* text, std::uint32_t* w, std::uint32_t* h) {
  unsigned a = 0;
  unsigned b = 0;
  if (std::sscanf(text, "%ux%u", &a, &b) != 2 || a < 160 || b < 120 || a > 4096 || b > 4096) {
    return false;
  }
  *w = a;
  *h = b;
  return true;
}

std::string Path(const std::string& dir, const char* name, int number = -1) {
  char buffer[64];
  if (number >= 0) {
    std::snprintf(buffer, sizeof buffer, "%s_%04d.png", name, number);
  } else {
    std::snprintf(buffer, sizeof buffer, "%s.png", name);
  }
  return dir + "/" + buffer;
}

}  // namespace

int main(int argc, char** argv) {
  std::uint64_t seed = 1;
  double seconds = 120.0;
  double every = 5.0;
  std::uint32_t width = 960;
  std::uint32_t height = 540;
  std::string out = ".";
  CockpitView view = CockpitView::kFront;
  int quality = 2;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const bool has_value = i + 1 < argc;
    if (arg == "--seed" && has_value) {
      seed = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--seconds" && has_value) {
      seconds = std::atof(argv[++i]);
    } else if (arg == "--every" && has_value) {
      every = std::atof(argv[++i]);
    } else if (arg == "--size" && has_value) {
      if (!ParseSize(argv[++i], &width, &height)) {
        std::fprintf(stderr, "bad size, expected WxH from 160x120 to 4096x4096\n");
        return 2;
      }
    } else if (arg == "--out" && has_value) {
      out = argv[++i];
    } else if (arg == "--quality" && has_value) {
      quality = std::atoi(argv[++i]);
      if (quality < 1 || quality > 4) {
        std::fprintf(stderr, "bad quality, expected 1 to 4\n");
        return 2;
      }
    } else if (arg == "--view" && has_value) {
      const std::string v = argv[++i];
      view = v == "rear" ? CockpitView::kRear : (v == "left" ? CockpitView::kLeft : (v == "right" ? CockpitView::kRight : CockpitView::kFront));
    } else {
      std::fprintf(stderr, "usage: flight_view [--seed N] [--seconds S] [--every SECONDS] [--size WxH] [--out DIRECTORY] [--view V] [--quality 1-4]\n");
      return 2;
    }
  }
  if (every <= 0.0) {
    every = 1.0 / 30.0;
  }

  Commander commander;
  commander.name = "Bot";
  commander.system = 40;
  commander.equipment.docking_computer = true;
  commander.equipment.ecm = true;
  commander.equipment.fuel_scoop = true;
  Game game(commander, seed);
  BotPilot bot(seed);
  Cockpit cockpit;
  cockpit.set_view(view);
  cockpit.set_supersample(quality);
  Image image(width, height);

  // The station screens at the start
  game.SelectSystem(game.galaxy().InRange(game.commander().system, kMaxFuel).front());
  const struct {
    const char* name;
    void (*draw)(Image&, const Game&);
  } screens[] = {
      {"status", [](Image& i, const Game& g) { DrawStatusScreen(i, g); }},
      {"market", [](Image& i, const Game& g) { DrawMarketScreen(i, g, 3); }},
      {"equipment", [](Image& i, const Game& g) { DrawEquipmentScreen(i, g, 1); }},
      {"chart", [](Image& i, const Game& g) { DrawChartScreen(i, g); }},
      {"system", [](Image& i, const Game& g) { DrawSystemScreen(i, g, g.selected_system()); }},
  };
  for (const auto& s : screens) {
    s.draw(image, game);
    if (!WritePng(image, Path(out, s.name))) {
      std::fprintf(stderr, "cannot write pictures into %s\n", out.c_str());
      return 1;
    }
  }

  double clock = 0.0;
  double next_shot = 1.0;
  int shots = 0;
  while (clock < seconds && game.mode() != GameMode::kDead) {
    if (game.mode() == GameMode::kDocked) {
      bot.Dock(game);
      game.Launch();
    }
    game.Update(bot.Fly(game), 1.0 / 60.0);
    clock += 1.0 / 60.0;
    cockpit.Notice(game, game.TakeEvents(), game.TakeMessages());
    cockpit.Advance(1.0 / 60.0);
    if (clock >= next_shot && game.mode() == GameMode::kFlight) {
      next_shot += every;
      cockpit.Draw(image, game, static_cast<std::uint32_t>(clock * 60.0));
      if (!WritePng(image, Path(out, "frame", shots++))) {
        std::fprintf(stderr, "cannot write pictures into %s\n", out.c_str());
        return 1;
      }
    }
  }
  std::printf("wrote %d flight pictures and 5 screens to %s\n", shots, out.c_str());
  return 0;
}
