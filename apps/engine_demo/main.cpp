// engine_demo: play the game with a computer pilot and print what happens. It exercises the whole engine (flight,
// combat, AI traffic, hyperspace, docking and trading) without a window.
//
//   engine_demo [--seed N] [--minutes M] [--newtonian] [--quiet]

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "coriolis/bot.h"
#include "coriolis/game.h"

using namespace coriolis;

int main(int argc, char** argv) {
  std::uint64_t seed = 1;
  double minutes = 20.0;
  bool newtonian = false;
  bool quiet = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const bool has_value = i + 1 < argc;
    if (arg == "--seed" && has_value) {
      seed = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--minutes" && has_value) {
      minutes = std::atof(argv[++i]);
    } else if (arg == "--newtonian") {
      newtonian = true;
    } else if (arg == "--quiet") {
      quiet = true;
    } else {
      std::fprintf(stderr, "usage: engine_demo [--seed N] [--minutes M] [--newtonian] [--quiet]\n");
      return 2;
    }
  }

  Commander commander;
  commander.name = "Bot";
  commander.system = 40;
  commander.equipment.docking_computer = true;
  commander.equipment.ecm = true;
  Game game(commander, seed);
  game.SetFlightMode(newtonian ? FlightMode::kNewtonian : FlightMode::kClassic);
  BotPilot bot(seed);

  double clock = 0.0;
  const double end = minutes * 60.0;
  while (clock < end && game.mode() != GameMode::kDead) {
    if (game.mode() == GameMode::kDocked) {
      bot.Dock(game);
      game.Launch();
    }
    game.Update(bot.Fly(game), 1.0 / 60.0);
    clock += 1.0 / 60.0;
    game.TakeEvents();
    for (const std::string& line : game.TakeMessages()) {
      if (!quiet) {
        std::printf("%7.1fs  %s\n", clock, line.c_str());
      }
    }
  }
  const Commander& c = game.commander();
  std::printf(
      "\n%.1f minutes played in %s: %d jumps, %d landings, %lld.%lld Cr, rank %s (%.1f kills), legal status %s%s\n",
      clock / 60.0, game.system().name.c_str(), bot.jumps(), bot.landings(), static_cast<long long>(c.credits / 10),
      static_cast<long long>(c.credits % 10), RankName(c.kills), c.kills, LegalName(c.legal_status),
      game.mode() == GameMode::kDead ? ", and died" : "");
  return 0;
}
