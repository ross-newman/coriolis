#include "open_elite/equipment_models.h"

#include <cmath>

#include "open_elite/model_builder.h"
#include "open_elite/model_parts.h"
#include "open_elite/ships.h"

namespace open_elite {

namespace {

using parts::Reversed;
using parts::Ring;
using parts::Tube;

/// The fuel: a fat orange tank with domed ends, a white band, a filler cap and a nozzle
Model BuildFuel() {
  ModelBuilder b("FUEL");
  const Rgb orange{226, 112, 40};
  const Rgb deep{190, 84, 30};
  const Rgb white{236, 236, 240};
  const Rgb steel{150, 154, 166};
  const Rgb dark{58, 56, 60};
  const std::vector<float> z{-44.0f, -38.0f, -30.0f, -8.0f, 8.0f, 30.0f, 38.0f, 44.0f};
  const std::vector<float> r{8.0f, 17.0f, 20.0f, 20.0f, 20.0f, 20.0f, 17.0f, 8.0f};
  b.Centre(0.0f, 0.0f, 0.0f);
  std::vector<Ring> rings;
  for (std::size_t i = 0; i < z.size(); ++i) {
    rings.push_back(b.Ring(10, r[i], r[i], z[i], kPi / 10.0f));
  }
  for (std::size_t i = 0; i + 1 < rings.size(); ++i) {
    b.Band(rings[i], rings[i + 1], i == 3 ? std::vector<Rgb>{white} : std::vector<Rgb>{orange, deep});
  }
  b.Poly(rings.back(), dark);
  b.Poly(Reversed(rings.front()), dark);
  // The filler cap on top and a nozzle on the side
  b.Box(-5.0f, 18.0f, -6.0f, 5.0f, 25.0f, 6.0f, steel, dark, false);
  b.Box(19.0f, -4.0f, 12.0f, 27.0f, 4.0f, 20.0f, steel, dark, false);
  return b.Finish(3.2f);
}

/// The E.C.M. system: a dark box with a dish on a mast and two antennae
Model BuildEcm() {
  ModelBuilder b("E.C.M. SYSTEM");
  const Rgb steel{112, 120, 136};
  const Rgb dark{54, 58, 70};
  const Rgb dish{196, 200, 212};
  const Rgb green{90, 230, 130};
  b.Box(-26.0f, -12.0f, -18.0f, 26.0f, 8.0f, 18.0f, steel, dark, false);
  b.Box(-3.0f, 8.0f, -3.0f, 3.0f, 24.0f, 3.0f, dark, dark, false);
  // The antennae on the top of the box
  b.Box(-19.5f, 8.0f, -9.0f, -16.5f, 30.0f, -6.0f, dark, dark, false);
  b.Box(16.5f, 8.0f, -9.0f, 19.5f, 30.0f, -6.0f, dark, dark, false);
  // The dish: a drum with a flat face and a raised hub in the middle
  b.Centre(0.0f, 36.0f, 0.0f);
  const Ring neck = b.Ring(12, 6.0f, 6.0f, -8.0f, 0.0f, 0.0f, 36.0f);
  const Ring rim = b.Ring(12, 26.0f, 26.0f, 6.0f, 0.0f, 0.0f, 36.0f);
  const Ring face = b.Ring(12, 17.0f, 17.0f, 6.0f, 0.0f, 0.0f, 36.0f);
  b.Band(neck, rim, {dish, steel});
  b.Band(rim, face, {steel});
  b.Poly(face, dark);
  b.Poly(Reversed(neck), dark);
  Tube(b, 8, 5.0f, 0.0f, 36.0f, 6.0f, 12.0f, {dish}, green, dark, true);
  // A light on the front of the box
  b.Centre(0.0f, -2.0f, 0.0f);
  b.Quad(b.Vertex(-20.0f, -8.0f, 18.4f), b.Vertex(-8.0f, -8.0f, 18.4f), b.Vertex(-8.0f, -3.0f, 18.4f),
         b.Vertex(-20.0f, -3.0f, 18.4f), green, true);
  return b.Finish(3.2f);
}

/// The fuel scoop: a wide funnel with a bright rim and a dark throat, and the pipe behind it
Model BuildFuelScoop() {
  ModelBuilder b("FUEL SCOOPS");
  const Rgb hull{168, 172, 186};
  const Rgb shade{118, 124, 142};
  const Rgb rim{236, 200, 84};
  const Rgb throat{16, 20, 30};
  b.Centre(0.0f, 0.0f, -4.0f);
  const Ring pipe_end = b.Ring(10, 12.0f, 12.0f, -52.0f, kPi / 10.0f);
  const Ring pipe = b.Ring(10, 12.0f, 12.0f, -30.0f, kPi / 10.0f);
  const Ring mouth = b.Ring(10, 36.0f, 36.0f, 22.0f, kPi / 10.0f);
  const Ring opening = b.Ring(10, 29.0f, 29.0f, 22.0f, kPi / 10.0f);
  b.Band(pipe_end, pipe, {shade, hull});
  b.Band(pipe, mouth, {hull, shade});
  b.Band(mouth, opening, {rim});
  b.Poly(opening, throat);
  b.Poly(Reversed(pipe_end), shade);
  // Two struts that brace the funnel
  b.Box(-2.0f, 10.0f, -42.0f, 2.0f, 30.0f, -22.0f, shade, shade, false);
  b.Box(-2.0f, -30.0f, -42.0f, 2.0f, -10.0f, -22.0f, shade, shade, false);
  return b.Finish(3.2f);
}

/// The energy bomb: a red ball with a yellow band, a fuse and four fins
Model BuildEnergyBomb() {
  ModelBuilder b("ENERGY BOMB");
  const Rgb red{200, 48, 44};
  const Rgb deep{150, 34, 34};
  const Rgb yellow{240, 200, 60};
  const Rgb dark{60, 56, 60};
  const float radius = 22.0f;
  b.Centre(0.0f, 0.0f, 0.0f);
  const std::size_t nose = b.Vertex(0.0f, 0.0f, radius);
  const std::size_t tail = b.Vertex(0.0f, 0.0f, -radius);
  std::vector<Ring> rings;
  for (int k = 1; k <= 4; ++k) {
    const float theta = kPi * static_cast<float>(k) / 5.0f;
    rings.push_back(b.Ring(10, radius * std::sin(theta), radius * std::sin(theta), radius * std::cos(theta), kPi / 10.0f));
  }
  b.Fan(nose, rings[0], {red, deep});
  b.Band(rings[0], rings[1], {red, deep});
  b.Band(rings[1], rings[2], {yellow});
  b.Band(rings[2], rings[3], {red, deep});
  b.Fan(tail, Reversed(rings[3]), {deep, red});
  // The fuse on the nose
  Tube(b, 8, 4.0f, 0.0f, 0.0f, radius - 3.0f, radius + 9.0f, {dark}, {255, 150, 60}, dark, true);
  // The tail fins
  b.Box(10.0f, -1.0f, -34.0f, 30.0f, 1.0f, -10.0f, dark, dark, false);
  b.Box(-30.0f, -1.0f, -34.0f, -10.0f, 1.0f, -10.0f, dark, dark, false);
  b.Box(-1.0f, 10.0f, -34.0f, 1.0f, 30.0f, -10.0f, dark, dark, false);
  b.Box(-1.0f, -30.0f, -34.0f, 1.0f, -10.0f, -10.0f, dark, dark, false);
  return b.Finish(3.2f);
}

/// The docking computer: a console with a screen showing a crosshair, two knobs and a base
Model BuildDockingComputer() {
  ModelBuilder b("DOCKING COMPUTER");
  const Rgb body{120, 126, 142};
  const Rgb dark{50, 54, 66};
  const Rgb screen{10, 34, 22};
  const Rgb green{90, 230, 130};
  b.Box(-28.0f, -18.0f, -14.0f, 28.0f, 12.0f, 14.0f, body, dark, false);
  b.Box(-36.0f, -24.0f, -20.0f, 36.0f, -18.0f, 20.0f, dark, dark, false);
  b.Box(-8.0f, 12.0f, -6.0f, 8.0f, 20.0f, 6.0f, dark, dark, false);
  // The knobs under the screen
  b.Box(-20.0f, -16.0f, 14.0f, -14.0f, -11.0f, 18.0f, dark, dark, false);
  b.Box(14.0f, -16.0f, 14.0f, 20.0f, -11.0f, 18.0f, dark, dark, false);
  // The screen and, on it, a crosshair and a square
  const float z = 14.4f;
  const float zg = 14.9f;
  b.Centre(0.0f, 0.0f, 0.0f);
  b.Quad(b.Vertex(-23.0f, -9.0f, z), b.Vertex(23.0f, -9.0f, z), b.Vertex(23.0f, 9.0f, z), b.Vertex(-23.0f, 9.0f, z), screen);
  b.Quad(b.Vertex(-16.0f, -0.8f, zg), b.Vertex(16.0f, -0.8f, zg), b.Vertex(16.0f, 0.8f, zg), b.Vertex(-16.0f, 0.8f, zg),
         green, true);
  b.Quad(b.Vertex(-0.8f, -6.0f, zg), b.Vertex(0.8f, -6.0f, zg), b.Vertex(0.8f, 6.0f, zg), b.Vertex(-0.8f, 6.0f, zg), green,
         true);
  b.Quad(b.Vertex(-6.0f, -4.0f, zg), b.Vertex(6.0f, -4.0f, zg), b.Vertex(6.0f, -3.0f, zg), b.Vertex(-6.0f, -3.0f, zg),
         green, true);
  b.Quad(b.Vertex(-6.0f, 3.0f, zg), b.Vertex(6.0f, 3.0f, zg), b.Vertex(6.0f, 4.0f, zg), b.Vertex(-6.0f, 4.0f, zg), green,
         true);
  return b.Finish(3.2f);
}

/// The beam laser: a long barrel with cooling fins, a glowing muzzle, a rear housing and a mount
Model BuildBeamLaser() {
  ModelBuilder b("BEAM LASER");
  const Rgb steel{150, 156, 170};
  const Rgb dark{54, 58, 70};
  const Rgb blue{70, 120, 210};
  b.Box(-12.0f, -12.0f, -46.0f, 12.0f, 12.0f, -24.0f, steel, dark, false);
  b.Box(-8.0f, -22.0f, -42.0f, 8.0f, -12.0f, -28.0f, dark, dark, false);
  b.Box(-13.0f, 3.0f, -46.0f, 13.0f, 6.0f, -24.0f, blue, blue, false);
  Tube(b, 8, 6.0f, 0.0f, 0.0f, -24.0f, 34.0f, {steel, dark}, dark, dark);
  for (const float z : {-12.0f, 2.0f, 16.0f}) {
    Tube(b, 8, 10.0f, 0.0f, 0.0f, z, z + 5.0f, {blue, steel}, blue, blue);
  }
  Tube(b, 8, 4.0f, 0.0f, 0.0f, 34.0f, 40.0f, {dark}, {150, 210, 255}, dark, true);
  return b.Finish(3.2f);
}

/// The military laser: two long barrels in shrouds, a heavy housing with cooling fins and red emitters
Model BuildMilitaryLaser() {
  ModelBuilder b("MILITARY LASER");
  const Rgb olive{96, 106, 84};
  const Rgb dark{46, 50, 44};
  const Rgb red{210, 56, 48};
  b.Box(-28.0f, -14.0f, -48.0f, 28.0f, 14.0f, -18.0f, olive, dark, false);
  b.Box(-10.0f, -24.0f, -44.0f, 10.0f, -14.0f, -24.0f, dark, dark, false);
  b.Box(-29.0f, -2.0f, -48.0f, 29.0f, 3.0f, -18.0f, red, red, false);
  for (const float x : {-18.0f, -6.0f, 6.0f, 18.0f}) {
    b.Box(x - 1.5f, 14.0f, -44.0f, x + 1.5f, 22.0f, -24.0f, dark, dark, false);
  }
  for (const float x : {-12.0f, 12.0f}) {
    Tube(b, 8, 6.0f, x, 0.0f, -18.0f, 44.0f, {olive, dark}, dark, dark);
    Tube(b, 8, 9.0f, x, 0.0f, -4.0f, 22.0f, {olive, dark}, dark, dark);
    Tube(b, 8, 4.5f, x, 0.0f, 44.0f, 50.0f, {dark}, {255, 120, 100}, dark, true);
  }
  return b.Finish(3.2f);
}

/// The models, in Item order
std::vector<Model> BuildAllItemModels() {
  std::vector<Model> items;
  items.push_back(GetShip(ShipId::kMissile));    // kMissile
  items.push_back(BuildEcm());                   // kEcm
  items.push_back(BuildFuelScoop());             // kFuelScoop
  items.push_back(BuildDockingComputer());       // kDockingComputer
  items.push_back(BuildEnergyBomb());            // kEnergyBomb
  items.push_back(GetShip(ShipId::kEscapePod));  // kEscapePod
  items.push_back(BuildBeamLaser());             // kBeamLaser
  items.push_back(BuildMilitaryLaser());         // kMilitaryLaser
  items.push_back(BuildFuel());                  // kFuel
  return items;
}

}  // namespace

const std::vector<Model>& AllItemModels() {
  static const std::vector<Model> items = BuildAllItemModels();
  return items;
}

const Model& GetItemModel(Item item) { return AllItemModels()[static_cast<std::size_t>(item)]; }

}  // namespace open_elite
