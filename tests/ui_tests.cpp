// Tests of the game as a player sees it: the title screen, the station screens, buying and selling, the chart, flight,
// pause, and saving. They drive GameUi with keys and flight input, with no window.

#include <cstdio>
#include <cstdlib>
#include <string>

#include "coriolis/bot.h"
#include "coriolis/commander.h"
#include "coriolis/screens.h"
#include "coriolis/ui.h"

namespace {

int g_failures = 0;

#define CHECK(condition)                                                                 \
  do {                                                                                   \
    if (!(condition)) {                                                                  \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
      ++g_failures;                                                                      \
    }                                                                                    \
  } while (false)

using namespace coriolis;

std::size_t LitAll(const Image& image) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < image.size(); i += 3) {
    n += (image.data()[i] > 24 || image.data()[i + 1] > 24 || image.data()[i + 2] > 24) ? 1 : 0;
  }
  return n;
}

/// A new game, in the station
struct Playing {
  GameUi ui;
  Image image{480, 270};
  Playing() : ui("", 5) { ui.Key(UiKey::kAccept); }  // the first item on the title is NEW GAME
  void Wait(double seconds) {
    for (int i = 0; i < static_cast<int>(seconds * 60.0); ++i) {
      ui.Update(FlightInput{}, 1.0 / 60.0);
    }
  }
};

void TheTitleScreenStartsANewGame() {
  GameUi ui("", 5);
  Image image(480, 270);
  CHECK(ui.screen() == Screen::kTitle);
  ui.Draw(image);
  CHECK(LitAll(image) > 2000);
  ui.Key(UiKey::kAccept);
  CHECK(ui.screen() == Screen::kStatus);
  CHECK(ui.game().mode() == GameMode::kDocked);
  CHECK(ui.game().system().name == "Lave");
  CHECK(!ui.wants_quit());
}

void QuitIsTheLastItemOnTheTitle() {
  GameUi ui("", 5);
  ui.Key(UiKey::kUp);  // wraps to QUIT
  ui.Key(UiKey::kAccept);
  CHECK(ui.wants_quit());
}

void TheStationScreensAreAllDrawnAndDiffer() {
  Playing p;
  Image shots[6] = {Image(480, 270), Image(480, 270), Image(480, 270), Image(480, 270), Image(480, 270), Image(480, 270)};
  const Screen order[] = {Screen::kStatus, Screen::kMarket, Screen::kEquipment, Screen::kShipyard, Screen::kChart, Screen::kSystem};
  for (int i = 0; i < 6; ++i) {
    CHECK(p.ui.screen() == order[i]);
    p.ui.Draw(shots[i]);
    CHECK(LitAll(shots[i]) > 1500);
    p.ui.Key(UiKey::kNextTab);
  }
  CHECK(p.ui.screen() == Screen::kStatus);
  for (int i = 1; i < 6; ++i) {
    CHECK(!(shots[0] == shots[i]));
  }
  p.ui.Key(UiKey::kPrevTab);
  CHECK(p.ui.screen() == Screen::kSystem);
}

void BuyingAndSellingCargo() {
  Playing p;
  p.ui.Key(UiKey::kNextTab);
  CHECK(p.ui.screen() == Screen::kMarket);
  // Find something on sale and buy it
  Commander& c = p.ui.game().commander();
  const std::int64_t cash = c.credits;
  bool bought = false;
  for (int row = 0; row < static_cast<int>(kCommodityCount) && !bought; ++row) {
    p.ui.Key(UiKey::kAccept);
    bought = CargoUsed(c) > 0;
    if (!bought) {
      p.ui.Key(UiKey::kDown);
    }
  }
  CHECK(bought);
  CHECK(c.credits < cash);
  CHECK(!p.ui.message().empty());
  p.ui.Key(UiKey::kAlternate);
  CHECK(CargoUsed(c) == 0);
  CHECK(c.credits == cash);
}

void BuyingEquipment() {
  Playing p;
  p.ui.Key(UiKey::kNextTab);
  p.ui.Key(UiKey::kNextTab);
  CHECK(p.ui.screen() == Screen::kEquipment);
  p.ui.game().commander().credits = 100000;
  p.ui.game().commander().fuel = 2.0;
  p.ui.Key(UiKey::kAccept);  // the first row is fuel
  CHECK(p.ui.game().commander().fuel > 2.0);
  // The second row is a missile
  p.ui.game().commander().missiles = 0;
  p.ui.Key(UiKey::kDown);
  p.ui.Key(UiKey::kAccept);
  CHECK(p.ui.game().commander().missiles == 1);
  // And with no money nothing is bought
  p.ui.game().commander().credits = 0;
  p.ui.Key(UiKey::kAccept);
  CHECK(p.ui.game().commander().missiles == 1);
}

void TheChartChoosesADestination() {
  Playing p;
  p.ui.Key(UiKey::kPrevTab);
  p.ui.Key(UiKey::kPrevTab);
  CHECK(p.ui.screen() == Screen::kChart);
  CHECK(p.ui.game().selected_system() < 0);
  p.ui.Key(UiKey::kRight);
  const int first = p.ui.game().selected_system();
  CHECK(first >= 0 && first != p.ui.game().commander().system);
  // Keep going the same way: the chosen system moves further along
  const StarSystem& a = p.ui.game().galaxy().system(first);
  p.ui.Key(UiKey::kRight);
  const int second = p.ui.game().selected_system();
  CHECK(second >= 0 && second != first);
  CHECK(p.ui.game().galaxy().system(second).x > a.x);
  p.ui.Key(UiKey::kAlternate);
  CHECK(p.ui.game().selected_system() < 0);
}

void LaunchingPausingAndTheChartInFlight() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  CHECK(p.ui.screen() == Screen::kFlight);
  CHECK(p.ui.game().mode() == GameMode::kFlight);
  p.Wait(2.0);
  p.ui.Draw(p.image);
  CHECK(LitAll(p.image) > 2000);

  // Pausing stops the game
  p.ui.Key(UiKey::kBack);
  CHECK(p.ui.paused());
  const Vec3d where = p.ui.game().player()->position;
  FlightInput throttle;
  throttle.throttle = 1.0f;
  for (int i = 0; i < 120; ++i) {
    p.ui.Update(throttle, 1.0 / 60.0);
  }
  CHECK(Distance(p.ui.game().player()->position, where) < 1e-9);
  Image paused(480, 270);
  p.ui.Draw(paused);
  CHECK(!(paused == p.image));
  p.ui.Key(UiKey::kAccept);  // RESUME
  CHECK(!p.ui.paused());
  p.Wait(1.0);
  CHECK(Distance(p.ui.game().player()->position, where) > 1.0);

  // The chart opens over the flight, takes the arrow keys, and closes
  FlightInput chart;
  chart.pressed.set(static_cast<std::size_t>(Action::kGalacticChart));
  p.ui.Update(chart, 1.0 / 60.0);
  CHECK(p.ui.chart_open());
  p.ui.Key(UiKey::kRight);
  CHECK(p.ui.game().selected_system() >= 0);
  p.ui.Key(UiKey::kBack);
  CHECK(!p.ui.chart_open());
  CHECK(!p.ui.paused());

  // Views change with the keys
  FlightInput rear;
  rear.pressed.set(static_cast<std::size_t>(Action::kViewRear));
  p.ui.Update(rear, 1.0 / 60.0);
  CHECK(p.ui.cockpit().view() == CockpitView::kRear);
  // The same key again looks front again; the front key always does
  p.ui.Update(rear, 1.0 / 60.0);
  CHECK(p.ui.cockpit().view() == CockpitView::kFront);
  FlightInput left;
  left.pressed.set(static_cast<std::size_t>(Action::kViewLeft));
  p.ui.Update(left, 1.0 / 60.0);
  CHECK(p.ui.cockpit().view() == CockpitView::kLeft);
  p.ui.Update(rear, 1.0 / 60.0);
  CHECK(p.ui.cockpit().view() == CockpitView::kRear);
  FlightInput front;
  front.pressed.set(static_cast<std::size_t>(Action::kViewFront));
  p.ui.Update(front, 1.0 / 60.0);
  CHECK(p.ui.cockpit().view() == CockpitView::kFront);
  p.ui.Key(UiKey::kFlightMode);
  CHECK(p.ui.game().flight_mode() == FlightMode::kNewtonian);
  p.ui.Key(UiKey::kFlightMode);
  CHECK(p.ui.game().flight_mode() == FlightMode::kClassic);
}

void QuittingToTheTitleAndResuming() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  p.Wait(1.0);
  p.ui.Key(UiKey::kBack);
  p.ui.Key(UiKey::kDown);  // QUIT TO TITLE
  p.ui.Key(UiKey::kAccept);
  CHECK(p.ui.screen() == Screen::kTitle);
  p.ui.Draw(p.image);
  // The first item is now RESUME
  p.ui.Key(UiKey::kAccept);
  CHECK(p.ui.screen() == Screen::kFlight);
  CHECK(!p.ui.paused());
}

void TheTractorBeamLandsTheShipAndThenTheStationScreensCome() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  p.Wait(7.0);  // out of the bay
  CHECK(p.ui.screen() == Screen::kFlight);
  Entity* me = p.ui.game().player();
  const Entity* station = p.ui.game().world().FindKind(EntityKind::kStation);
  CHECK(me != nullptr && station != nullptr);
  if (me == nullptr || station == nullptr) {
    return;
  }
  const Bay bay = BayOf(*station);
  me->position = DockPoint(*station) + bay.axis * 30.0;
  me->orientation = LookAlong(-bay.axis, bay.up);
  me->velocity = -bay.axis * 40.0;
  double flying_after_beam = 0.0;
  bool beam = false;
  for (int i = 0; i < 60 * 20 && p.ui.screen() == Screen::kFlight; ++i) {
    p.ui.Update(FlightInput{}, 1.0 / 60.0);
    if (p.ui.game().landing()) {
      beam = true;
      flying_after_beam += 1.0 / 60.0;
      if (i % 30 == 0) {
        p.ui.Draw(p.image);
        CHECK(LitAll(p.image) > 1000);
      }
    }
  }
  // The flight screen stays up while the beam lands the ship; the station screens follow by themselves
  CHECK(beam);
  CHECK(flying_after_beam > 4.0);
  CHECK(p.ui.screen() == Screen::kStatus);
  CHECK(p.ui.game().mode() == GameMode::kDocked);
}

void AFullRoundTripSavesAndContinues() {
  const std::string path = "/tmp/coriolis_ui_test_save.json";
  std::remove(path.c_str());
  // The computer pilot can be shot down by a pirate before it gets back, as a person can: so try a few seeds
  bool round_trip = false;
  for (std::uint64_t seed = 11; seed < 21 && !round_trip; ++seed) {
    std::remove(path.c_str());
    GameUi ui(path, seed);
    ui.Key(UiKey::kAccept);
    // The computer pilot lands with the docking computer, so fit one
    ui.game().commander().equipment.docking_computer = true;
    ui.game().commander().equipment.ecm = true;
    BotPilot bot(seed);
    bot.Dock(ui.game());
    ui.Key(UiKey::kLaunch);
    int dockings = 0;
    for (int frame = 0; frame < 60 * 600 && dockings < 1; ++frame) {
      if (ui.screen() == Screen::kFlight) {
        ui.Update(bot.Fly(ui.game()), 1.0 / 60.0);
      } else if (ui.screen() == Screen::kDead) {
        break;
      } else {
        // Docked again: the game saved by itself
        ++dockings;
      }
    }
    if (dockings != 1) {
      continue;
    }
    round_trip = true;
    CHECK(ui.screen() == Screen::kStatus);
    Commander saved;
    std::string error;
    CHECK(LoadCommander(path, &saved, &error));
    CHECK(saved.system == ui.game().commander().system);
    CHECK(saved.credits == ui.game().commander().credits);
  }
  CHECK(round_trip);
  // A new run finds the save, and the title offers it
  GameUi again(path, 12);
  again.Key(UiKey::kAccept);  // CONTINUE SAVED GAME is first
  CHECK(again.screen() == Screen::kStatus);
  CHECK(again.game().mode() == GameMode::kDocked);
  CHECK(again.game().commander().name == "Jameson");
  std::remove(path.c_str());
}

void ExplicitSaveInTheStation() {
  const std::string path = "/tmp/coriolis_ui_test_save2.json";
  std::remove(path.c_str());
  GameUi ui(path, 3);
  ui.Key(UiKey::kAccept);
  ui.game().commander().credits = 424242;
  ui.Key(UiKey::kSave);
  Commander loaded;
  std::string error;
  CHECK(LoadCommander(path, &loaded, &error));
  CHECK(loaded.credits == 424242);
  std::remove(path.c_str());
}

void EveryScreenDrawsAtAnySize() {
  Playing p;
  for (const std::uint32_t w : {1u, 64u, 160u, 320u, 800u}) {
    Image image(w, w * 9 / 16 + 1);
    p.ui.Draw(image);
    for (int i = 0; i < 5; ++i) {
      p.ui.Key(UiKey::kNextTab);
      p.ui.Draw(image);
    }
  }
  p.ui.Key(UiKey::kLaunch);
  p.Wait(0.5);
  for (const std::uint32_t w : {1u, 64u, 160u, 800u}) {
    Image image(w, w * 9 / 16 + 1);
    p.ui.Draw(image);
  }
}

}  // namespace

void DyingShowsTheEndScreenWithItsCauseAndChoices() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  CHECK(p.ui.screen() == Screen::kFlight);
  p.Wait(7.0);  // out of the bay
  Entity* me = p.ui.game().player();
  const Blueprint* mamba = FindBuiltInBlueprint("mamba");
  const EntityId killer =
      p.ui.game().world().SpawnShip(*mamba, Team::kPirate, me->position + Vec3d{0.0, 0.0, 800.0}, Quat{});
  me = p.ui.game().player();
  p.ui.game().world().ApplyDamage(*me, 1e9, killer, me->position);
  p.Wait(0.2);
  CHECK(p.ui.screen() == Screen::kDead);
  CHECK(p.ui.game().death_cause().find("MAMBA") != std::string::npos);

  // Drawn at any size, and it changes as the wreck flies apart and the lines come in
  Image first(480, 270);
  p.ui.Draw(first);
  CHECK(LitAll(first) > 500);
  p.Wait(3.0);
  Image later(480, 270);
  p.ui.Draw(later);
  CHECK(!(first == later));
  Image wide(1280, 720);
  p.ui.Draw(wide);
  CHECK(LitAll(wide) > 2000);
  Image tiny(64, 40);
  p.ui.Draw(tiny);

  // The choices: with no save there is NEW GAME, TITLE SCREEN and QUIT, and Escape goes to the title
  p.ui.Key(UiKey::kDown);
  p.ui.Key(UiKey::kUp);
  p.ui.Key(UiKey::kAccept);
  CHECK(p.ui.screen() == Screen::kStatus);
  CHECK(p.ui.game().mode() == GameMode::kDocked);
}

void TheEndScreenIgnoresKeysAtFirst() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  p.Wait(7.0);  // out of the bay
  Entity* me = p.ui.game().player();
  p.ui.game().world().ApplyDamage(*me, 1e9, kNoEntity, me->position);
  p.Wait(0.2);
  CHECK(p.ui.screen() == Screen::kDead);
  // A press meant for the game must not skip the screen
  p.ui.Key(UiKey::kAccept);
  p.ui.Key(UiKey::kBack);
  CHECK(p.ui.screen() == Screen::kDead);
  p.Wait(1.5);
  p.ui.Key(UiKey::kBack);
  CHECK(p.ui.screen() == Screen::kTitle);
}

void LaunchingRidesOutOfTheBay() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  CHECK(p.ui.game().launching());
  const Entity& station = *p.ui.game().world().FindKind(EntityKind::kStation);
  const Bay bay = BayOf(station);
  // At the start the ship is deep inside the bay, at rest
  const Vec3d start = p.ui.game().player()->position - bay.mouth;
  CHECK(Dot(start, bay.axis) < -0.5 * bay.depth);
  CHECK(p.ui.game().player()->Speed() < 1.0);
  // The controls are locked and nothing can hurt the ship
  FlightInput hard;
  hard.pitch = 1.0f;
  hard.throttle = -1.0f;
  Image inside(480, 270);
  p.ui.Draw(inside);
  CHECK(LitAll(inside) > 3000);
  for (int i = 0; i < 180; ++i) {
    p.ui.Update(hard, 1.0 / 60.0);
  }
  CHECK(p.ui.game().launching());
  const Vec3d later = p.ui.game().player()->position - bay.mouth;
  CHECK(Dot(later, bay.axis) > Dot(start, bay.axis) + 50.0);
  CHECK(std::fabs(Dot(later, bay.right)) < 1.0 && std::fabs(Dot(later, bay.up)) < 1.0);
  // It comes out of the mouth, then the pilot has the ship, flying free and fast
  p.Wait(4.0);
  CHECK(!p.ui.game().launching());
  CHECK(Dot(p.ui.game().player()->position - bay.mouth, bay.axis) > 50.0);
  CHECK(p.ui.game().player()->Speed() > 100.0);
  CHECK(!p.ui.game().player()->ghost);
  // The view while riding out and after is drawn
  Image outside(480, 270);
  p.ui.Draw(outside);
  CHECK(!(inside == outside));
}

void TheHelpListPausesTheGame() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  p.Wait(7.0);
  Image flying(480, 270);
  p.ui.Draw(flying);
  p.ui.Key(UiKey::kHelp);
  CHECK(p.ui.help_open());
  const Vec3d where = p.ui.game().player()->position;
  p.Wait(1.0);
  CHECK(Distance(p.ui.game().player()->position, where) < 1e-9);
  Image help(480, 270);
  p.ui.Draw(help);
  CHECK(!(help == flying));
  // Drawn at any size
  Image tiny(64, 40);
  p.ui.Draw(tiny);
  Image wide(1280, 720);
  p.ui.Draw(wide);
  // Any key closes it, and the game goes on
  p.ui.Key(UiKey::kAccept);
  CHECK(!p.ui.help_open());
  p.Wait(1.0);
  CHECK(Distance(p.ui.game().player()->position, where) > 1.0);
}

void TargetAndThrottleKeys() {
  Playing p;
  p.ui.Key(UiKey::kLaunch);
  p.Wait(7.0);
  Game& game = p.ui.game();
  const Vec3d at = game.player()->position;
  const EntityId near_ship = game.world().SpawnShip(*FindBuiltInBlueprint("viper"), Team::kPolice, at + Vec3d{0, 0, 2000}, Quat{});
  game.world().Find(near_ship)->manual = true;
  const EntityId far_ship = game.world().SpawnShip(*FindBuiltInBlueprint("viper"), Team::kPolice, at + Vec3d{0, 0, 5000}, Quat{});
  game.world().Find(far_ship)->manual = true;
  const auto press = [&](Action a) {
    FlightInput in;
    in.pressed.set(static_cast<std::size_t>(a));
    p.ui.Update(in, 1.0 / 60.0);
  };
  // Other traffic may be about too, so check the order of the ring of targets rather than which is which
  press(Action::kNextTarget);
  const EntityId first = game.target();
  CHECK(first != kNoEntity);
  press(Action::kNextTarget);
  const EntityId second = game.target();
  CHECK(second != kNoEntity && second != first);
  press(Action::kPreviousTarget);
  CHECK(game.target() == first);
  press(Action::kPreviousTarget);
  const EntityId last = game.target();  // wraps round to the farthest
  CHECK(last != kNoEntity && last != first);
  press(Action::kNextTarget);
  CHECK(game.target() == first);
  CHECK(near_ship != far_ship);
  // Throttle straight to full and straight to nothing
  press(Action::kThrottleFull);
  CHECK(game.throttle() > 0.99);
  press(Action::kThrottleZero);
  CHECK(game.throttle() < 0.01);
}

int main() {
  TheHelpListPausesTheGame();
  TargetAndThrottleKeys();
  LaunchingRidesOutOfTheBay();
  TheTitleScreenStartsANewGame();
  QuitIsTheLastItemOnTheTitle();
  TheStationScreensAreAllDrawnAndDiffer();
  BuyingAndSellingCargo();
  BuyingEquipment();
  TheChartChoosesADestination();
  LaunchingPausingAndTheChartInFlight();
  QuittingToTheTitleAndResuming();
  TheTractorBeamLandsTheShipAndThenTheStationScreensCome();
  AFullRoundTripSavesAndContinues();
  ExplicitSaveInTheStation();
  DyingShowsTheEndScreenWithItsCauseAndChoices();
  TheEndScreenIgnoresKeysAtFirst();
  EveryScreenDrawsAtAnySize();
  if (g_failures == 0) {
    std::printf("all ui tests passed\n");
    return 0;
  }
  std::fprintf(stderr, "%d ui test checks failed\n", g_failures);
  return 1;
}
