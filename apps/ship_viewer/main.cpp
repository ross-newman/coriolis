// ship_viewer: draw the ships to PNG files.
//
//   ship_viewer [--ship N|all] [--size WxH] [--angle RADIANS] [--out DIRECTORY] [--stars]
//   ship_viewer --showcase SECONDS [--size WxH] [--out DIRECTORY]
//
// With --ship the named ship (0 to 12) is drawn; with --ship all each of the 13 ships is. --showcase writes the
// pictures of the showcase at 0, 1, 2 ... SECONDS.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "open_elite/png.h"
#include "open_elite/renderer.h"
#include "open_elite/ships.h"
#include "open_elite/showcase.h"

namespace {

void Usage() {
  std::fprintf(stderr,
               "usage: ship_viewer [--ship N|all] [--size WxH] [--angle RADIANS] [--out DIRECTORY] [--no-stars]\n"
               "       ship_viewer --showcase SECONDS [--size WxH] [--out DIRECTORY]\n");
}

}  // namespace

int main(int argc, char** argv) {
  std::string ship = "all";
  std::string out = ".";
  std::uint32_t width = 640;
  std::uint32_t height = 360;
  float angle = 2.36f;
  bool stars = true;
  int showcase_seconds = -1;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const bool has_value = i + 1 < argc;
    if (arg == "--ship" && has_value) {
      ship = argv[++i];
    } else if (arg == "--size" && has_value) {
      if (std::sscanf(argv[++i], "%ux%u", &width, &height) != 2 || width == 0 || height == 0) {
        Usage();
        return 2;
      }
    } else if (arg == "--angle" && has_value) {
      angle = static_cast<float>(std::atof(argv[++i]));
    } else if (arg == "--out" && has_value) {
      out = argv[++i];
    } else if (arg == "--showcase" && has_value) {
      showcase_seconds = std::atoi(argv[++i]);
    } else if (arg == "--no-stars") {
      stars = false;
    } else {
      Usage();
      return 2;
    }
  }

  using namespace open_elite;
  int failures = 0;

  if (showcase_seconds >= 0) {
    const Showcase showcase;
    Image image(width, height);
    for (int second = 0; second <= showcase_seconds; ++second) {
      showcase.Render(image, second);
      char name[64];
      std::snprintf(name, sizeof name, "/showcase_%04d.png", second);
      failures += WritePng(image, out + name) ? 0 : 1;
    }
    return failures == 0 ? 0 : 1;
  }

  const Renderer renderer;
  for (std::size_t index = 0; index < kShipCount; ++index) {
    if (ship != "all" && static_cast<std::size_t>(std::atoi(ship.c_str())) != index) {
      continue;
    }
    const Model& model = AllShips()[index];
    Image image(width, height);
    if (stars) {
      DrawStars(image, 0);
    }
    renderer.DrawShip(image, model, angle);
    DrawCaption(image, model.name);
    char name[32];
    std::snprintf(name, sizeof name, "/ship_%02zu.png", index);
    const bool ok = WritePng(image, out + name);
    std::printf("%s %s%s\n", ok ? "wrote" : "FAILED", (out + name).c_str(), ok ? "" : "");
    failures += ok ? 0 : 1;
  }
  return failures == 0 ? 0 : 1;
}
