// Tests for the ship renderer. A small self-contained runner so there are no dependencies.

#include <cmath>
#include <cstdio>
#include <set>
#include <string>

#include "open_elite/model_builder.h"
#include "open_elite/png.h"
#include "open_elite/renderer.h"
#include "open_elite/ships.h"
#include "open_elite/showcase.h"

namespace {

int g_failures = 0;

#define CHECK(condition)                                                                 \
  do {                                                                                   \
    if (!(condition)) {                                                                  \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
      ++g_failures;                                                                      \
    }                                                                                    \
  } while (false)

using namespace open_elite;

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

void ThereAreThirteenShips() {
  CHECK(AllShips().size() == kShipCount);
  CHECK(kShipCount == 13);
  std::set<std::string> names;
  for (const Model& ship : AllShips()) {
    CHECK(!ship.name.empty());
    CHECK(!ship.vertices.empty());
    CHECK(!ship.faces.empty());
    names.insert(ship.name);
  }
  // The two Cobra Mk III entries are named apart
  CHECK(names.size() == kShipCount);
  CHECK(GetShip(ShipId::kCobraMk3Trader).name == "COBRA MK III TRADER");
  CHECK(GetShip(ShipId::kCobraMk3BountyHunter).name == "COBRA MK III BOUNTY HUNTER");
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
  CHECK(showcase.ShipAt(30.0 * 13) == 0);
  CHECK(showcase.ShipAt(30.0 * 13 + 40.0) == 1);

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
  CHECK(WritePng(image, "open_elite_test.png"));
  std::FILE* f = std::fopen("open_elite_test.png", "rb");
  CHECK(f != nullptr);
  if (f != nullptr) {
    unsigned char signature[8] = {};
    CHECK(std::fread(signature, 1, 8, f) == 8);
    CHECK(signature[1] == 'P' && signature[2] == 'N' && signature[3] == 'G');
    std::fclose(f);
    std::remove("open_elite_test.png");
  }
}

}  // namespace

int main() {
  ThereAreThirteenShips();
  ShipsAreCentredAndSizedAlike();
  EveryShipDrawsAndTheyDiffer();
  DrawingIsRepeatableAndTurns();
  ShipsDrawOverTheBackground();
  StarsAreRepeatableAndDrift();
  TextDrawsInsideTheImage();
  ShowcaseCyclesEveryDwellTime();
  ShowcaseShowsTheShip();
  PngIsWritten();
  if (g_failures == 0) {
    std::printf("all tests passed\n");
  }
  return g_failures == 0 ? 0 : 1;
}
