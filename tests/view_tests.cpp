// Tests for the flight screen and the station screens. They check what is drawn by counting pixels, so they need no
// window and no reference pictures.

#include <cmath>
#include <cstdio>
#include <string>

#include "coriolis/cockpit.h"
#include "coriolis/game.h"
#include "coriolis/renderer.h"
#include "coriolis/screens.h"

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

/// Pixels in a block that are not black
std::size_t Lit(const Image& image, std::uint32_t x0, std::uint32_t y0, std::uint32_t x1, std::uint32_t y1) {
  std::size_t n = 0;
  for (std::uint32_t y = y0; y < y1; ++y) {
    for (std::uint32_t x = x0; x < x1; ++x) {
      const std::uint8_t* p = image.data() + (static_cast<std::size_t>(y) * image.width() + x) * 3;
      n += (p[0] > 24 || p[1] > 24 || p[2] > 24) ? 1 : 0;
    }
  }
  return n;
}

std::size_t LitAll(const Image& image) { return Lit(image, 0, 0, image.width(), image.height()); }

/// A game in flight, just launched
struct Flying {
  Game game;
  Cockpit cockpit;
  Image image{640, 360};
  explicit Flying(int system = 40) : game(MakeCommander(system), 3) {
    game.Launch(false);
    cockpit.set_nebula(false);  // the tests below count lit pixels against a black sky
  }
  static Commander MakeCommander(int system = 40) {
    Commander c;
    c.system = system;
    return c;
  }
  void Draw() { cockpit.Draw(image, game, 0); }
  Entity& Me() { return *game.player(); }
  const Entity& Station() { return *game.world().FindKind(EntityKind::kStation); }
};

void NothingIsDrawnOutsideFlight() {
  Game docked(Flying::MakeCommander(), 3);
  Cockpit cockpit;
  Image image(320, 200);
  image.Clear({9, 9, 9});
  cockpit.Draw(image, docked, 0);
  CHECK(LitAll(image) == 0);
}

void TheStationShowsWhenYouLookAtIt() {
  Flying f;
  const Entity& st = f.Station();
  const Vec3d axis = DockAxis(st);
  const Vec3d dock = DockPoint(st);
  // 1.5 km out in front of the bay, looking at it, then looking the other way
  f.Me().position = dock + axis * 1500.0;
  f.Me().velocity = Vec3d{};
  f.Me().orientation = LookAlong(-axis, {0, 1, 0});
  f.Draw();
  const std::size_t toward = Lit(f.image, 160, 70, 480, 210);
  f.Me().orientation = LookAlong(axis, {0, 1, 0});
  f.Draw();
  const std::size_t away = Lit(f.image, 160, 70, 480, 210);
  // A solid station fills a good part of the middle of the view; the other way there are only stars
  CHECK(toward > 8000);
  CHECK(away < 600);
}

void ThePlanetIsASolidBall() {
  Flying f;
  const Entity* planet = f.game.world().FindKind(EntityKind::kPlanet);
  const Entity* sun = f.game.world().FindKind(EntityKind::kSun);
  CHECK(planet != nullptr && sun != nullptr);
  // Hover low over the side of the planet that faces the sun, looking down: the lit ground fills the view
  const Vec3d up = Normalized(sun->position - planet->position);
  f.Me().position = planet->position + up * (planet->radius + 3000.0);
  f.Me().velocity = Vec3d{};
  f.Me().orientation = LookAlong(-up, {0, 0, 1});
  f.Draw();
  CHECK(Lit(f.image, 160, 70, 480, 210) > 320 * 140 * 8 / 10);
  // On the night side it is dark: nothing to see but a black ball hiding the stars
  f.Me().position = planet->position - up * (planet->radius + 3000.0);
  f.Me().orientation = LookAlong(up, {0, 0, 1});
  f.Draw();
  CHECK(Lit(f.image, 160, 70, 480, 210) < 320 * 140 / 10);
}

void TheSunShines() {
  Flying f;
  const Entity* sun = f.game.world().FindKind(EntityKind::kSun);
  CHECK(sun != nullptr);
  f.Me().position = sun->position + Vec3d{0, 0, -150000.0};
  f.Me().velocity = Vec3d{};
  f.Me().orientation = LookAlong({0, 0, 1}, {0, 1, 0});
  f.Draw();
  // A bright disc with a glow, in the middle
  std::size_t bright = 0;
  for (std::uint32_t y = 100; y < 180; ++y) {
    for (std::uint32_t x = 280; x < 360; ++x) {
      const std::uint8_t* p = f.image.data() + (static_cast<std::size_t>(y) * f.image.width() + x) * 3;
      bright += p[0] > 200 ? 1 : 0;
    }
  }
  CHECK(bright > 500);
}

void DrawingIsRepeatableAndOrderIndependent() {
  Flying a;
  Flying b;
  a.Draw();
  b.Draw();
  CHECK(a.image == b.image);
  a.Draw();
  CHECK(a.image == b.image);
}

void ShipsAreDrawnAsShapesAndAsDots() {
  Flying f;
  f.Me().velocity = Vec3d{};
  const Vec3d at = f.Me().position;
  const Vec3d fwd = Forward(f.Me().orientation);
  f.Draw();
  const std::size_t empty = Lit(f.image, 220, 90, 420, 200);
  // A python 120 m ahead is a big solid shape
  const EntityId id = f.game.world().SpawnShip(*FindBuiltInBlueprint("python"), Team::kNeutral, at + fwd * 120.0, LookAlong(-fwd + Vec3d{0.3, 0, 0}, {0, 1, 0}));
  f.game.world().Find(id)->manual = true;
  f.Draw();
  const std::size_t near_lit = Lit(f.image, 220, 90, 420, 200);
  CHECK(near_lit > empty + 1500);
  // 40 km away it is a dot or two, and nothing more while it is no threat
  f.game.world().Find(id)->position = at + fwd * 40000.0;
  f.Draw();
  const std::size_t far_lit = Lit(f.image, 220, 90, 420, 200);
  CHECK(far_lit < empty + 30);
  CHECK(far_lit >= empty);
  // Once it is after the player it gets a yellow marker round it, which makes it easy to find
  f.game.world().Find(id)->ai.target = f.Me().id;
  f.game.world().Find(id)->ai.mode = AiMode::kAttack;
  f.Draw();
  const std::size_t marked = Lit(f.image, 220, 90, 420, 200);
  CHECK(marked > far_lit + 8);
  CHECK(marked < far_lit + 150);
  // Right up against the camera it still draws (the near plane cuts it) and does not crash
  f.game.world().Find(id)->position = at + fwd * 3.0;
  f.Draw();
  f.game.world().Find(id)->position = at - fwd * 3.0;
  f.Draw();
}

void SolidShipsHideWhatIsBehindThem() {
  Flying f;
  f.Me().velocity = Vec3d{};
  const Vec3d at = f.Me().position;
  const Vec3d fwd = Forward(f.Me().orientation);
  // A station-sized ship hides a small one behind it
  const Blueprint& big = *FindBuiltInBlueprint("anaconda");
  const Blueprint& small = *FindBuiltInBlueprint("missile");
  const EntityId a = f.game.world().SpawnShip(big, Team::kNeutral, at + fwd * 150.0, LookAlong(Vec3d{0.2, 0, -1}, {0, 1, 0}));
  f.game.world().Find(a)->manual = true;
  f.Draw();
  const Image alone = f.image;
  const EntityId b = f.game.world().SpawnShip(small, Team::kNeutral, at + fwd * 400.0, Quat{});
  f.game.world().Find(b)->manual = true;
  f.Draw();
  // The missile behind is hidden by the anaconda in front: the view is the same (the scanner and the little plan
  // position indicator in the top right do see it, so they are left out)
  bool same = true;
  for (std::uint32_t y = 0; y < 270 && same; ++y) {
    for (std::uint32_t x = 0; x < 540 && same; ++x) {
      for (std::size_t k = 0; k < 3; ++k) {
        same = same && f.image.data()[(y * 640 + x) * 3 + k] == alone.data()[(y * 640 + x) * 3 + k];
      }
    }
  }
  CHECK(same);
}

void TheDashboardShowsTheShip() {
  Flying f;
  f.Draw();
  const std::uint32_t panel_top = 288;
  // The console takes the bottom fifth and has computer screens on it
  CHECK(Lit(f.image, 0, panel_top + 4, 640, 360) > 8000);
  // Above it there is no frame, pillars or roof: only the view (here mostly stars) below the lines of text
  CHECK(Lit(f.image, 0, 40, 640, panel_top - 1) < 1500);
  // The console starts on the line, not above it
  CHECK(Lit(f.image, 0, panel_top - 3, 640, panel_top - 1) < 100);
  const Image full = f.image;
  // Shields down: the bar empties and the picture changes
  f.Me().shield = 0.0;
  f.Draw();
  CHECK(!(f.image == full));
  // The scanner shows other ships: add one and the scanner changes
  const Image before = f.image;
  const EntityId id = f.game.world().SpawnShip(*FindBuiltInBlueprint("viper"), Team::kPolice, f.Me().position + Vec3d{2000, 0, 3000}, Quat{});
  f.game.world().Find(id)->manual = true;
  f.Draw();
  CHECK(!(f.image == before));
  // Messages and the hyperspace countdown appear
  Image quiet(640, 360);
  f.cockpit.Draw(quiet, f.game, 0);
  f.cockpit.Notice(f.game, {}, {"A MESSAGE FOR YOU"});
  Image shown(640, 360);
  f.cockpit.Draw(shown, f.game, 0);
  CHECK(!(shown == quiet));
  f.cockpit.Advance(7.0);
  Image faded(640, 360);
  f.cockpit.Draw(faded, f.game, 0);
  CHECK(faded == quiet);
}

void TargetsGetBracketsAndALeadMarker() {
  Flying f;
  f.Me().velocity = Vec3d{};
  const Vec3d fwd = Forward(f.Me().orientation);
  const EntityId id = f.game.world().SpawnShip(*FindBuiltInBlueprint("mamba"), Team::kPirate, f.Me().position + fwd * 1800.0, Quat{});
  f.game.world().Find(id)->manual = true;
  f.Draw();
  const Image without = f.image;
  FlightInput press;
  press.pressed.set(static_cast<std::size_t>(Action::kNextTarget));
  f.game.Update(press, 1.0 / 60.0);
  CHECK(f.game.target() == id);
  f.Draw();
  CHECK(!(f.image == without));
  // Locking turns the brackets red
  const Image blue = f.image;
  FlightInput lock;
  lock.pressed.set(static_cast<std::size_t>(Action::kTargetMissile));
  f.game.Update(lock, 1.0 / 60.0);
  f.Draw();
  CHECK(!(f.image == blue));
}

void ExplosionsFlashAndFade() {
  Flying f;
  f.Me().velocity = Vec3d{};
  f.Draw();
  const Image quiet = f.image;
  Event boom;
  boom.kind = EventKind::kDestroyed;
  boom.a = 12345;
  boom.position = f.Me().position + Forward(f.Me().orientation) * 400.0;
  boom.blueprint = FindBuiltInBlueprint("python");
  f.cockpit.Notice(f.game, {boom}, {});
  f.cockpit.Advance(0.2);
  f.Draw();
  CHECK(LitAll(f.image) > LitAll(quiet) + 1500);
  f.cockpit.Advance(2.0);
  f.Draw();
  CHECK(f.image == quiet);
  // A hit on the player tints the edge of the view
  Event hit;
  hit.kind = EventKind::kHullHit;
  hit.a = f.game.world().player();
  f.cockpit.Notice(f.game, {hit}, {});
  f.Draw();
  CHECK(!(f.image == quiet));
}

void EveryViewDrawsAndTheyDiffer() {
  Flying f;
  const Entity& st = f.Station();
  f.Me().position = DockPoint(st) + DockAxis(st) * 2500.0;
  f.Me().orientation = LookAlong(-DockAxis(st), {0, 1, 0});
  Image pictures[4] = {Image(640, 360), Image(640, 360), Image(640, 360), Image(640, 360)};
  const CockpitView views[] = {CockpitView::kFront, CockpitView::kRear, CockpitView::kLeft, CockpitView::kRight};
  for (int i = 0; i < 4; ++i) {
    f.cockpit.set_view(views[i]);
    CHECK(f.cockpit.view() == views[i]);
    f.cockpit.Draw(pictures[i], f.game, 0);
  }
  CHECK(!(pictures[0] == pictures[1]));
  CHECK(!(pictures[0] == pictures[2]));
  CHECK(!(pictures[2] == pictures[3]));
  // The station is ahead, so it is only in the front view
  CHECK(Lit(pictures[0], 160, 70, 480, 210) > 3 * Lit(pictures[1], 160, 70, 480, 210));
}

void ThePlanPositionIndicatorShowsTheView() {
  Flying f;
  f.Me().velocity = Vec3d{};
  const auto corner = [](const Image& image) {
    Image out(100, 50);
    for (std::uint32_t y = 0; y < 50; ++y) {
      for (std::uint32_t x = 0; x < 100; ++x) {
        for (std::size_t k = 0; k < 3; ++k) {
          out.data()[(y * 100 + x) * 3 + k] = image.data()[((20 + y) * image.width() + 540 + x) * 3 + k];
        }
      }
    }
    return out;
  };
  f.Draw();
  const Image front = corner(f.image);
  // There is a lit glass in the corner, and the wedge swings round with the view
  CHECK(Lit(f.image, 560, 22, 640, 66) > 100);
  f.cockpit.set_view(CockpitView::kRear);
  f.Draw();
  CHECK(!(corner(f.image) == front));
  f.cockpit.set_view(CockpitView::kFront);
  f.Draw();
  CHECK(corner(f.image) == front);
  // It shows the camera and nothing else: a ship coming up behind does not change it
  f.game.world().Find(f.game.world().SpawnShip(*FindBuiltInBlueprint("viper"), Team::kPolice,
                                                f.Me().position - Forward(f.Me().orientation) * 3000.0, Quat{}))
      ->manual = true;
  f.Draw();
  CHECK(corner(f.image) == front);
}

void ButtonsBesideTheScreensShowWhatIsFitted() {
  Flying f;
  f.Me().velocity = Vec3d{};
  const auto edge = [](const Image& image) {
    // The strip along the left edge of the console, where the first column of buttons is
    Image out(34, 50);
    for (std::uint32_t y = 0; y < 50; ++y) {
      for (std::uint32_t x = 0; x < 34; ++x) {
        for (std::size_t k = 0; k < 3; ++k) {
          out.data()[(y * 34 + x) * 3 + k] = image.data()[((310 + y) * image.width() + x) * 3 + k];
        }
      }
    }
    return out;
  };
  f.game.commander().equipment.escape_pod = false;
  f.game.commander().equipment.ecm = false;
  f.game.commander().equipment.fuel_scoop = false;
  f.Draw();
  const Image bare = edge(f.image);
  f.game.commander().equipment.escape_pod = true;
  f.game.commander().equipment.fuel_scoop = true;
  f.Draw();
  // A button that was dark is lit now that the equipment is fitted, and it stays lit
  CHECK(!(edge(f.image) == bare));
  const Image fitted = edge(f.image);
  f.Draw();
  CHECK(edge(f.image) == fitted);
}

void TheSkyHasGasClouds() {
  // Every system has one cloud. Look straight at the brightest part of it; the planet or the sun may happen to be in
  // the way in some systems, so try a few until one shows it.
  bool seen = false;
  for (int system = 0; system < 30 && !seen; ++system) {
    const Nebula sky(Game(Flying::MakeCommander(system), 3).system().seed);
    Vec3d best{0.0, 0.0, 1.0};
    float peak = 0.0f;
    for (int i = 0; i < 180; ++i) {
      for (int j = 1; j < 90; ++j) {
        const double yaw = i * 2.0 * kPi / 180.0;
        const double pitch = j * kPi / 90.0 - kPi / 2.0;
        const Vec3d d{std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw)};
        const auto light = sky.Sample(d);
        const float level = light[0] + light[1] + light[2];
        if (level > peak) {
          peak = level;
          best = d;
        }
      }
    }
    CHECK(peak > 30.0f);
    Flying f(system);
    f.Me().orientation = LookAlong(best, std::fabs(best.y) > 0.95 ? Vec3d{0, 0, 1} : Vec3d{0, 1, 0});
    f.cockpit.set_nebula(false);
    f.Draw();
    const Image clear = f.image;
    f.cockpit.set_nebula(true);
    f.Draw();
    const Image with_gas = f.image;
    if (!(with_gas == clear)) {
      seen = true;
      // It is the same every time
      f.Draw();
      CHECK(f.image == with_gas);
    }
  }
  CHECK(seen);
}

void TinyAndHugePicturesAreSafe() {
  Flying f;
  for (const std::uint32_t w : {1u, 100u, 160u, 200u, 1280u}) {
    Image image(w, w * 9 / 16 + 1);
    f.cockpit.Draw(image, f.game, 5);
  }
}

void ALongFlightWithEverythingGoingOnDraws() {
  Game game(Flying::MakeCommander(), 8);
  game.Launch(false);
  Cockpit cockpit;
  Image image(480, 270);
  std::size_t drawn = 0;
  for (int frame = 0; frame < 60 * 90 && game.mode() == GameMode::kFlight; ++frame) {
    FlightInput in;
    in.throttle = 0.4f;
    in.pitch = static_cast<float>(std::sin(frame * 0.02));
    in.roll = static_cast<float>(std::cos(frame * 0.013));
    in.held.set(static_cast<std::size_t>(Action::kFireLasers));
    game.Update(in, 1.0 / 60.0);
    if (game.player() != nullptr) {
      game.player()->invulnerable = true;
    }
    cockpit.Notice(game, game.TakeEvents(), game.TakeMessages());
    cockpit.Advance(1.0 / 60.0);
    if (frame % 30 == 0) {
      cockpit.Draw(image, game, static_cast<std::uint32_t>(frame));
      ++drawn;
    }
  }
  CHECK(drawn >= 100);
}

void TextHasDigitsAndLowerCase() {
  Image digits(200, 20);
  DrawText(digits, "0123456789", 2, 2, 2, {255, 255, 255});
  CHECK(LitAll(digits) > 200);
  Image upper(100, 20);
  Image lower(100, 20);
  DrawText(upper, "HELLO WORLD: 3.5 KM!", 1, 1, 1, {255, 255, 255});
  DrawText(lower, "hello world: 3.5 km!", 1, 1, 1, {255, 255, 255});
  CHECK(upper == lower);
  // Each digit is different from the others
  Image a(8, 8);
  Image b(8, 8);
  for (char c = '0'; c <= '8'; ++c) {
    Image one(8, 8);
    Image next(8, 8);
    DrawText(one, std::string(1, c), 1, 0, 1, {255, 255, 255});
    DrawText(next, std::string(1, static_cast<char>(c + 1)), 1, 0, 1, {255, 255, 255});
    CHECK(!(one == next));
  }
  (void)a;
  (void)b;
}

void TheStationScreensShowTheGame() {
  Commander c;
  c.system = 40;
  c.credits = 98765;
  c.cargo[3] = 4;
  c.equipment.ecm = true;
  Game game(c, 3);
  game.SelectSystem(game.galaxy().InRange(40, 7.0).front());
  Image status(640, 400);
  Image market(640, 400);
  Image equipment(640, 400);
  Image chart(640, 400);
  Image system(640, 400);
  DrawStatusScreen(status, game);
  DrawMarketScreen(market, game, 2);
  DrawEquipmentScreen(equipment, game, 1);
  DrawChartScreen(chart, game);
  DrawSystemScreen(system, game, game.selected_system());
  for (const Image* i : {&status, &market, &equipment, &chart, &system}) {
    CHECK(LitAll(*i) > 1500);
  }
  // The chart has a dot for each of the 256 systems
  CHECK(LitAll(chart) > 600);
  // Different games give different screens
  Commander richer = c;
  richer.credits = 5;
  Game poor(richer, 3);
  Image status2(640, 400);
  DrawStatusScreen(status2, poor);
  CHECK(!(status == status2));
  // The highlighted row of the market moves
  Image market2(640, 400);
  DrawMarketScreen(market2, game, 5);
  CHECK(!(market == market2));
  CHECK(EquipmentCount() >= 8);
  // The screens cope with small pictures
  Image tiny(80, 60);
  DrawStatusScreen(tiny, game);
  DrawMarketScreen(tiny, game, 0);
  DrawEquipmentScreen(tiny, game, 0);
  DrawChartScreen(tiny, game);
  DrawSystemScreen(tiny, game, 9999);
}

void SmoothEdgesAndTheCockpitFollowTheShip() {
  Flying f;
  f.Me().velocity = Vec3d{};
  Image plain(320, 180);
  Image smooth(320, 180);
  f.cockpit.Draw(plain, f.game, 0);
  f.cockpit.set_supersample(3);
  CHECK(f.cockpit.supersample() == 3);
  f.cockpit.Draw(smooth, f.game, 0);
  CHECK(!(plain == smooth));
  CHECK(LitAll(smooth) > 3000);
  // Turning hard shifts the cockpit a little, then it settles
  f.cockpit.set_supersample(1);
  Image still(320, 180);
  f.cockpit.Draw(still, f.game, 0);
  f.Me().spin = Vec3d{0.0, 0.0, 1.5};
  for (int i = 0; i < 10; ++i) {
    f.cockpit.Draw(plain, f.game, 0);
  }
  CHECK(!(plain == still));
}

}  // namespace

int main() {
  NothingIsDrawnOutsideFlight();
  SmoothEdgesAndTheCockpitFollowTheShip();
  TheStationShowsWhenYouLookAtIt();
  ThePlanetIsASolidBall();
  TheSunShines();
  DrawingIsRepeatableAndOrderIndependent();
  ShipsAreDrawnAsShapesAndAsDots();
  SolidShipsHideWhatIsBehindThem();
  TheDashboardShowsTheShip();
  TargetsGetBracketsAndALeadMarker();
  ExplosionsFlashAndFade();
  EveryViewDrawsAndTheyDiffer();
  TheSkyHasGasClouds();
  ThePlanPositionIndicatorShowsTheView();
  ButtonsBesideTheScreensShowWhatIsFitted();
  TinyAndHugePicturesAreSafe();
  ALongFlightWithEverythingGoingOnDraws();
  TextHasDigitsAndLowerCase();
  TheStationScreensShowTheGame();
  if (g_failures == 0) {
    std::printf("all view tests passed\n");
  }
  return g_failures == 0 ? 0 : 1;
}
