// Tests for the ship renderer. A small self-contained runner so there are no dependencies.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>

#include "coriolis/commodity_models.h"
#include "coriolis/equipment_models.h"
#include "coriolis/input.h"
#include "coriolis/model_builder.h"
#include "coriolis/nebula.h"
#include "coriolis/obj.h"
#include "coriolis/png.h"
#include "coriolis/renderer.h"
#include "coriolis/ships.h"
#include "coriolis/showcase.h"

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

std::size_t LitPixels(const Image& image) {
  std::size_t lit = 0;
  for (std::size_t i = 0; i < image.size(); ++i) {
    lit += image.data()[i] > 32 ? 1 : 0;
  }
  return lit;
}

Image Draw(std::size_t ship, float angle) {
  Image image(320, 240);
  Renderer().DrawShip(image, AllShips()[ship], angle);
  return image;
}

void ThereAreAllTheShips() {
  CHECK(AllShips().size() == kShipCount);
  CHECK(kShipCount == 36);
  CHECK(static_cast<std::size_t>(ShipId::kEliteLogo) + 1 == kShipCount);
  // The wheel is a much bigger station than the others
  CHECK(GetShip(ShipId::kWheelStation).size_metres >= 3.0f * GetShip(ShipId::kCoriolis).size_metres);
  CHECK(GetShip(ShipId::kWheelStation).spin == Spin::kRoll);
  std::set<std::string> names;
  for (const Model& ship : AllShips()) {
    CHECK(!ship.name.empty());
    CHECK(!ship.vertices.empty());
    CHECK(!ship.faces.empty());
    names.insert(ship.name);
  }
  // The Cobra Mk III and Python entries are named apart
  CHECK(names.size() == kShipCount);
  CHECK(GetShip(ShipId::kCobraMk3Pirate).name == "COBRA MK III PIRATE");
  CHECK(GetShip(ShipId::kPythonPirate).name == "PYTHON PIRATE");
  CHECK(GetShip(ShipId::kEliteLogo).name == "ELITE LOGO");
  CHECK(GetShip(ShipId::kCobraMk3Trader).name == "COBRA MK III TRADER");
  CHECK(GetShip(ShipId::kCobraMk3BountyHunter).name == "COBRA MK III BOUNTY HUNTER");
  CHECK(GetShip(ShipId::kFerDeLance).name == "FER-DE-LANCE");
  CHECK(GetShip(ShipId::kDodo).name == "DODO STATION");
}

void ShipsAreCentredAndSizedAlike() {
  for (const Model& ship : AllShips()) {
    Vec3 low{1e9f, 1e9f, 1e9f};
    Vec3 high{-1e9f, -1e9f, -1e9f};
    float largest = 0.0f;
    for (const Vec3& v : ship.vertices) {
      low = {std::fmin(low.x, v.x), std::fmin(low.y, v.y), std::fmin(low.z, v.z)};
      high = {std::fmax(high.x, v.x), std::fmax(high.y, v.y), std::fmax(high.z, v.z)};
      largest = std::fmax(largest, std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
    }
    CHECK(std::fabs(low.x + high.x) < 0.01f);
    CHECK(std::fabs(low.y + high.y) < 0.01f);
    CHECK(std::fabs(low.z + high.z) < 0.01f);
    CHECK(largest > 2.9f && largest < 3.3f);
    for (const Face& face : ship.faces) {
      for (const std::size_t v : face.v) {
        CHECK(v < ship.vertices.size());
      }
    }
  }
}

void EveryShipDrawsAndTheyDiffer() {
  std::set<std::string> pictures;
  for (std::size_t i = 0; i < kShipCount; ++i) {
    const Image image = Draw(i, 1.0f);
    CHECK(LitPixels(image) > 500);
    pictures.insert(std::string(reinterpret_cast<const char*>(image.data()), image.size()));
  }
  CHECK(pictures.size() == kShipCount);
}

void EveryThingForSaleHasAModel() {
  CHECK(AllItemModels().size() == kItemCount);
  CHECK(static_cast<std::size_t>(Item::kFuel) + 1 == kItemCount);
  // The missile and the escape pod are the ships of the same name
  CHECK(GetItemModel(Item::kMissile).name == GetShip(ShipId::kMissile).name);
  CHECK(GetItemModel(Item::kEscapePod).name == GetShip(ShipId::kEscapePod).name);
  CHECK(GetItemModel(Item::kBeamLaser).name == "BEAM LASER");
  CHECK(GetItemModel(Item::kMilitaryLaser).name == "MILITARY LASER");
  CHECK(GetItemModel(Item::kEcm).name == "E.C.M. SYSTEM");
  CHECK(GetItemModel(Item::kFuelScoop).name == "FUEL SCOOPS");
  CHECK(GetItemModel(Item::kDockingComputer).name == "DOCKING COMPUTER");
  CHECK(GetItemModel(Item::kEnergyBomb).name == "ENERGY BOMB");
  CHECK(GetItemModel(Item::kFuel).name == "FUEL");
  std::set<std::string> names;
  std::set<std::string> pictures;
  for (const Model& item : AllItemModels()) {
    CHECK(!item.vertices.empty());
    CHECK(!item.faces.empty());
    names.insert(item.name);
    Vec3 low{1e9f, 1e9f, 1e9f};
    Vec3 high{-1e9f, -1e9f, -1e9f};
    float largest = 0.0f;
    for (const Vec3& v : item.vertices) {
      low = {std::fmin(low.x, v.x), std::fmin(low.y, v.y), std::fmin(low.z, v.z)};
      high = {std::fmax(high.x, v.x), std::fmax(high.y, v.y), std::fmax(high.z, v.z)};
      largest = std::fmax(largest, std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
    }
    // Centred and the same size as the ships
    CHECK(std::fabs(low.x + high.x) < 0.01f && std::fabs(low.y + high.y) < 0.01f && std::fabs(low.z + high.z) < 0.01f);
    CHECK(largest > 2.9f && largest < 3.3f);
    for (const Face& face : item.faces) {
      for (const std::size_t v : face.v) {
        CHECK(v < item.vertices.size());
      }
    }
    Image image(320, 240);
    Renderer().DrawShip(image, item, 2.4f);
    CHECK(LitPixels(image) > 500);
    pictures.insert(std::string(reinterpret_cast<const char*>(image.data()), image.size()));
  }
  CHECK(names.size() == kItemCount);
  CHECK(pictures.size() == kItemCount);
}

void EveryGoodHasAModel() {
  CHECK(AllCommodityModels().size() == kCommodityCount);
  std::set<std::string> names;
  std::set<std::string> pictures;
  for (std::size_t i = 0; i < kCommodityCount; ++i) {
    const Commodity good = static_cast<Commodity>(i);
    const Model& model = GetCommodityModel(good);
    // Named as on the market screen, which shows the names in capitals
    std::string market_name = CommodityData(good).name;
    for (char& ch : market_name) {
      ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    CHECK(model.name == market_name);
    CHECK(!model.vertices.empty() && !model.faces.empty());
    names.insert(model.name);
    Vec3 low{1e9f, 1e9f, 1e9f};
    Vec3 high{-1e9f, -1e9f, -1e9f};
    float largest = 0.0f;
    for (const Vec3& v : model.vertices) {
      low = {std::fmin(low.x, v.x), std::fmin(low.y, v.y), std::fmin(low.z, v.z)};
      high = {std::fmax(high.x, v.x), std::fmax(high.y, v.y), std::fmax(high.z, v.z)};
      largest = std::fmax(largest, std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
    }
    CHECK(std::fabs(low.x + high.x) < 0.01f && std::fabs(low.y + high.y) < 0.01f && std::fabs(low.z + high.z) < 0.01f);
    CHECK(largest > 2.9f && largest < 3.3f);
    for (const Face& face : model.faces) {
      for (const std::size_t v : face.v) {
        CHECK(v < model.vertices.size());
      }
    }
    Image image(320, 240);
    Renderer().DrawShip(image, model, 2.4f);
    CHECK(LitPixels(image) > 500);
    pictures.insert(std::string(reinterpret_cast<const char*>(image.data()), image.size()));
  }
  CHECK(names.size() == kCommodityCount);
  CHECK(pictures.size() == kCommodityCount);
}

void DrawingIsRepeatableAndTurns() {
  CHECK(Draw(4, 1.0f) == Draw(4, 1.0f));
  CHECK(!(Draw(4, 1.0f) == Draw(4, 1.5f)));
}

void ShipsDrawOverTheBackground() {
  Image image(320, 240);
  image.Clear({10, 20, 30});
  Renderer().DrawShip(image, AllShips()[0], 1.0f);
  // The corner is untouched
  CHECK(image.data()[0] == 10 && image.data()[1] == 20 && image.data()[2] == 30);
}

void StarsAreRepeatableAndDrift() {
  Image a(320, 240);
  Image b(320, 240);
  Image c(320, 240);
  DrawStars(a, 5);
  DrawStars(b, 5);
  DrawStars(c, 50);
  CHECK(a == b);
  CHECK(!(a == c));
  CHECK(LitPixels(a) > 100);
}

void NebulaIsRepeatableVariedAndDim() {
  const Quat look;
  Image a(320, 240);
  Image b(320, 240);
  Nebula(7).Paint(a, look, 250.0, 160.0, 120.0);
  Nebula(7).Paint(b, look, 250.0, 160.0, 120.0);
  CHECK(a == b);
  // Different seeds give different skies (the clouds are small, so turn round to find them)
  bool differ = false;
  for (int k = 0; k < 24 * 7 && !differ; ++k) {
    const Quat turn = Normalized(FromAxisAngle({0.0, 1.0, 0.0}, (k % 24) * 0.2618) *
                                 FromAxisAngle({1.0, 0.0, 0.0}, (k / 24 - 3) * 0.4));
    Image p(320, 240);
    Image q(320, 240);
    Nebula(7).Paint(p, turn, 250.0, 160.0, 120.0);
    Nebula(8).Paint(q, turn, 250.0, 160.0, 120.0);
    differ = !(p == q);
  }
  CHECK(differ);

  // Over the whole sky, for a run of seeds: gas is found in most systems, it covers only a small part of the sky and
  // it stays dim, so it never drowns the stars
  int with_gas = 0;
  for (std::uint32_t seed = 1; seed <= 20; ++seed) {
    const Nebula nebula(seed, 64);
    int samples = 0;
    int covered = 0;
    float peak = 0.0f;
    for (int i = 0; i < 160; ++i) {
      for (int j = 0; j < 80; ++j) {
        const double yaw = i * 2.0 * kPi / 160.0;
        const double pitch = (j + 0.5) / 80.0 * kPi - kPi / 2.0;
        const auto light = nebula.Sample({std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw)});
        const float level = std::max({light[0], light[1], light[2]});
        peak = std::max(peak, level);
        covered += level > 8.0f ? 1 : 0;
        // Only blues, indigos, purples and pinks: some blue in it, and never greener than it is red or blue
        if (level > 8.0f) {
          CHECK(light[2] >= 0.5f * light[0]);
          CHECK(light[1] <= 0.95f * std::max(light[0], light[2]));
        }
        ++samples;
      }
    }
    CHECK(peak < 140.0f);
    CHECK(covered * 100 < samples * 8);
    with_gas += peak > 20.0f ? 1 : 0;
  }
  // Every system has its cloud
  CHECK(with_gas == 20);

  // The flat version is repeatable
  Image flat(320, 240);
  Image again(320, 240);
  DrawNebula(flat, 3, 0);
  DrawNebula(again, 3, 0);
  CHECK(flat == again);
}

void TextDrawsInsideTheImage() {
  Image image(200, 100);
  DrawText(image, "VIPER", 4, 4, 2, {255, 255, 255});
  CHECK(LitPixels(image) > 50);
  // Text off the edge is clipped, not a crash
  DrawText(image, "MAMBA", -10, 95, 3, {255, 255, 255});
  DrawText(image, "MAMBA", 195, -5, 3, {255, 255, 255});
}

void ShowcaseCyclesEveryDwellTime() {
  const Showcase showcase;
  CHECK(showcase.ShipAt(0.0) == 0);
  CHECK(showcase.ShipAt(29.9) == 0);
  CHECK(showcase.ShipAt(30.0) == 1);
  CHECK(showcase.ShipAt(5.0 + 30.0 * 12) == 12);
  // After the last ship the first comes round again
  CHECK(showcase.ShipAt(30.0 * static_cast<double>(kShipCount)) == 0);
  CHECK(showcase.ShipAt(30.0 * static_cast<double>(kShipCount) + 40.0) == 1);

  ShowcaseOptions fast;
  fast.seconds_per_ship = 10.0;
  CHECK(Showcase(fast).ShipAt(25.0) == 2);

  ShowcaseOptions fixed;
  fixed.fixed_ship = 7;
  CHECK(Showcase(fixed).ShipAt(0.0) == 7);
  CHECK(Showcase(fixed).ShipAt(1000.0) == 7);
}

void ShowcaseShowsTheShip() {
  const Showcase showcase;
  Image image(320, 240);
  showcase.Render(image, 35.0);
  CHECK(LitPixels(image) > 500);
  ShowcaseOptions fixed;
  fixed.fixed_ship = 1;
  Image fixed_image(320, 240);
  Showcase(fixed).Render(fixed_image, 35.0);
  CHECK(image == fixed_image);
}

void PngIsWritten() {
  Image image(64, 48);
  image.Clear({1, 2, 3});
  CHECK(WritePng(image, "coriolis_test.png"));
  std::FILE* f = std::fopen("coriolis_test.png", "rb");
  CHECK(f != nullptr);
  if (f != nullptr) {
    unsigned char signature[8] = {};
    CHECK(std::fread(signature, 1, 8, f) == 8);
    CHECK(signature[1] == 'P' && signature[2] == 'N' && signature[3] == 'G');
    std::fclose(f);
    std::remove("coriolis_test.png");
  }
}

bool Near(float a, float b, float tolerance = 1e-4f) { return std::fabs(a - b) <= tolerance; }

void SticksHaveADeadZoneAndReachFull() {
  const InputConfig config = DefaultInputConfig(GamepadKind::kGeneric);
  float x = 9.0f;
  float y = 9.0f;
  // Inside the dead zone reads as centred
  ShapeStick(config, 0.1f, -0.1f, &x, &y);
  CHECK(x == 0.0f && y == 0.0f);
  // The dead zone is a circle, not a square: this point is inside it although neither axis is tiny
  ShapeStick(config, 0.1f, 0.1f, &x, &y);
  CHECK(x == 0.0f && y == 0.0f);
  // Full deflection gives 1, in any direction, and no more
  ShapeStick(config, 1.0f, 0.0f, &x, &y);
  CHECK(Near(x, 1.0f) && Near(y, 0.0f));
  ShapeStick(config, 0.0f, -1.0f, &x, &y);
  CHECK(Near(x, 0.0f) && Near(y, -1.0f));
  ShapeStick(config, 0.8f, 0.8f, &x, &y);
  CHECK(Near(std::sqrt(x * x + y * y), 1.0f));
  CHECK(Near(x, y));
  // Direction is kept just past the dead zone, and the output grows smoothly from zero
  ShapeStick(config, 0.2f, 0.0f, &x, &y);
  CHECK(x > 0.0f && x < 0.05f && y == 0.0f);
  // Garbage from a driver is ignored
  ShapeStick(config, std::nanf(""), 1.0f, &x, &y);
  CHECK(x == 0.0f && y == 0.0f);
}

void Xbox360HasABiggerDeadZoneThanElite() {
  CHECK(DefaultInputConfig(GamepadKind::kXbox360).stick_dead_zone >
        DefaultInputConfig(GamepadKind::kXboxElite).stick_dead_zone);
}

void SticksFlyTheShip() {
  InputMapper mapper(DefaultInputConfig(GamepadKind::kXboxElite));
  GamepadState pad;
  // Pull the left stick back: nose up. The right stick left: yaw left.
  pad.left_y = -1.0f;
  pad.right_x = -1.0f;
  FlightInput in = mapper.Update(pad, 0.016f);
  CHECK(Near(in.pitch, 1.0f));
  CHECK(Near(in.roll, 0.0f));
  CHECK(Near(in.yaw, -1.0f));
  CHECK(Near(in.throttle, 0.0f));

  // The right stick up: more throttle. The left stick to the right: roll right.
  GamepadState throttle_up;
  throttle_up.right_y = 1.0f;
  CHECK(Near(mapper.Update(throttle_up, 0.016f).throttle, 1.0f));
  GamepadState roll_right;
  roll_right.left_x = 1.0f;
  CHECK(Near(mapper.Update(roll_right, 0.016f).roll, 1.0f));

  InputConfig inverted = DefaultInputConfig(GamepadKind::kXboxElite);
  inverted.invert_pitch = true;
  InputMapper flipped(inverted);
  CHECK(Near(flipped.Update(pad, 0.016f).pitch, -1.0f));

  // Centred sticks give nothing
  CHECK(mapper.Update(GamepadState(), 0.016f).pitch == 0.0f);
}

void ButtonsGivePressAndReleaseOnce() {
  InputMapper mapper(DefaultInputConfig(GamepadKind::kXbox360));
  GamepadState pad;
  pad.Set(GamepadButton::kLeftShoulder);
  FlightInput in = mapper.Update(pad, 0.016f);
  CHECK(in.Pressed(Action::kEcm) && in.Held(Action::kEcm));
  // Still down: held but not pressed again
  in = mapper.Update(pad, 0.016f);
  CHECK(!in.Pressed(Action::kEcm) && in.Held(Action::kEcm));
  pad.Set(GamepadButton::kLeftShoulder, false);
  in = mapper.Update(pad, 0.016f);
  CHECK(in.Released(Action::kEcm) && !in.Held(Action::kEcm));
  in = mapper.Update(pad, 0.016f);
  CHECK(!in.Released(Action::kEcm));
}

void TriggersFireWithAGapBeforeTheyLetGo() {
  InputMapper mapper(DefaultInputConfig(GamepadKind::kXbox360));
  GamepadState pad;
  pad.right_trigger = 0.4f;
  CHECK(!mapper.Update(pad, 0.016f).Held(Action::kFireLasers));
  pad.right_trigger = 0.6f;
  CHECK(mapper.Update(pad, 0.016f).Pressed(Action::kFireLasers));
  // Easing off to between the two limits keeps firing
  pad.right_trigger = 0.4f;
  CHECK(mapper.Update(pad, 0.016f).Held(Action::kFireLasers));
  pad.right_trigger = 0.2f;
  CHECK(mapper.Update(pad, 0.016f).Released(Action::kFireLasers));
  // The left trigger is the missile
  pad.left_trigger = 1.0f;
  CHECK(mapper.Update(pad, 0.016f).Pressed(Action::kFireMissile));
}

void EjectingNeedsAHold() {
  InputMapper mapper(DefaultInputConfig(GamepadKind::kXbox360));
  GamepadState pad;
  pad.Set(GamepadButton::kRightStick);
  CHECK(!mapper.Update(pad, 0.5f).Held(Action::kEscapePod));
  FlightInput in = mapper.Update(pad, 0.6f);
  CHECK(in.Pressed(Action::kEscapePod));
  // Letting go and pressing again starts the hold over
  pad.Set(GamepadButton::kRightStick, false);
  CHECK(mapper.Update(pad, 0.016f).Released(Action::kEscapePod));
  pad.Set(GamepadButton::kRightStick);
  CHECK(!mapper.Update(pad, 0.5f).Held(Action::kEscapePod));
}

void ElitePaddlesDoActions() {
  InputMapper mapper(DefaultInputConfig(GamepadKind::kXboxElite));
  GamepadState pad;
  pad.Set(GamepadButton::kPaddleRightUpper);
  pad.Set(GamepadButton::kPaddleLeftUpper);
  const FlightInput in = mapper.Update(pad, 0.016f);
  CHECK(in.Pressed(Action::kToggleFlightAssist));
  CHECK(in.Pressed(Action::kEnergyBomb));
  CHECK(!in.Pressed(Action::kNextTarget));
}

void EveryButtonOfTheDefaultLayoutIsUsed() {
  const InputConfig config = DefaultInputConfig(GamepadKind::kXboxElite);
  std::set<GamepadButton> bound;
  for (const ButtonBinding& b : config.bindings) {
    CHECK(b.hold_seconds >= 0.0f);
    CHECK(bound.insert(b.button).second);
  }
  // Every button has something to do, including the paddles and triggers. Each is bound once.
  CHECK(bound.size() == kGamepadButtonCount);
}

// A cube the way most tools write one: right handed, counter clockwise from outside, mixed face forms, one face with
// negative indices
const char kCubeObj[] =
    "# a cube\n"
    "v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\nv -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n"
    "vn 0 0 1\nvt 0 0\n"
    "f 1//1 4//1 3//1 2//1\n"
    "f 5/1/1 6/1/1 7/1/1 8/1/1\n"
    "f 1 2 6 5\n"
    "f 4 8 7 3\n"
    "f 1 5 8 4\n"
    "f -7 -6 -2 -3\n";

void ObjCubeLoadsWithOutwardFaces() {
  Model cube;
  std::string error;
  CHECK(ParseObj(kCubeObj, "", {}, &cube, &error));
  CHECK(cube.vertices.size() == 8);
  CHECK(cube.faces.size() == 12);
  for (const Face& f : cube.faces) {
    const Vec3& a = cube.vertices[f.v[0]];
    const Vec3& b = cube.vertices[f.v[1]];
    const Vec3& c = cube.vertices[f.v[2]];
    const Vec3 n{(b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y),
                 (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z),
                 (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)};
    const Vec3 middle{(a.x + b.x + c.x) / 3.0f, (a.y + b.y + c.y) / 3.0f, (a.z + b.z + c.z) / 3.0f};
    // Pointing away from the middle of the cube, which is the origin, and from the face's own centre
    CHECK(n.x * middle.x + n.y * middle.y + n.z * middle.z > 0.0f);
    CHECK(n.x * (middle.x - f.centre.x) + n.y * (middle.y - f.centre.y) + n.z * (middle.z - f.centre.z) > 0.0f);
  }
  for (const Vec3& v : cube.vertices) {
    CHECK(std::fabs(std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) - 3.2f) < 0.01f);
  }
  Image image(320, 240);
  Renderer().DrawShip(image, cube, 1.0f);
  CHECK(LitPixels(image) > 500);
}

void ObjNoseAlongMinusZEndsUpAlongPlusZ() {
  // A pyramid whose apex is at z = -1, which is the front in a "-Z forward" file
  const char pyramid[] =
      "v -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\nv 0 0 -1\n"
      "f 1 2 3 4\nf 1 5 2\nf 2 5 3\nf 3 5 4\nf 4 5 1\n";
  Model flipped;
  CHECK(ParseObj(pyramid, "", {}, &flipped, nullptr));
  ObjOptions keep;
  keep.flip_z = false;
  Model kept;
  CHECK(ParseObj(pyramid, "", keep, &kept, nullptr));
  const auto apex_z = [](const Model& m) {
    for (const Vec3& v : m.vertices) {
      if (std::fabs(v.x) < 1e-4f && std::fabs(v.y) < 1e-4f) {
        return v.z;
      }
    }
    return 0.0f;
  };
  CHECK(apex_z(flipped) > 1.0f);
  CHECK(apex_z(kept) < -1.0f);
}

void ObjMistakesAreReported() {
  Model model;
  std::string error;
  CHECK(!ParseObj("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 9\n", "", {}, &model, &error));
  CHECK(error.find("line 4") != std::string::npos);
  CHECK(!ParseObj("v 0 0 0\n", "", {}, &model, &error));
  CHECK(!ParseObj("v 0 0\n", "", {}, &model, &error));
  CHECK(!ParseObj("v 0 0 0\nv 1 0 0\nf 1 2\n", "", {}, &model, &error));
  CHECK(!LoadObj("/nonexistent/model.obj", {}, &model, &error));
}

}  // namespace

int main() {
  ThereAreAllTheShips();
  ShipsAreCentredAndSizedAlike();
  EveryShipDrawsAndTheyDiffer();
  EveryThingForSaleHasAModel();
  EveryGoodHasAModel();
  DrawingIsRepeatableAndTurns();
  ShipsDrawOverTheBackground();
  StarsAreRepeatableAndDrift();
  NebulaIsRepeatableVariedAndDim();
  TextDrawsInsideTheImage();
  ShowcaseCyclesEveryDwellTime();
  ShowcaseShowsTheShip();
  PngIsWritten();
  SticksHaveADeadZoneAndReachFull();
  Xbox360HasABiggerDeadZoneThanElite();
  SticksFlyTheShip();
  ButtonsGivePressAndReleaseOnce();
  TriggersFireWithAGapBeforeTheyLetGo();
  EjectingNeedsAHold();
  ElitePaddlesDoActions();
  EveryButtonOfTheDefaultLayoutIsUsed();
  ObjCubeLoadsWithOutwardFaces();
  ObjNoseAlongMinusZEndsUpAlongPlusZ();
  ObjMistakesAreReported();
  if (g_failures == 0) {
    std::printf("all tests passed\n");
  }
  return g_failures == 0 ? 0 : 1;
}
