// ship_viewer: draw the ships to PNG files.
//
//   ship_viewer [--ship N|all] [--size WxH] [--angle RADIANS] [--out DIRECTORY] [--stars]
//   ship_viewer --obj FILE.obj [--keep-z] [--flip-winding] [--sway] [--size WxH] [--angle RADIANS] [--out DIRECTORY]
//   ship_viewer --items [--size WxH] [--angle RADIANS] [--out DIRECTORY]
//   ship_viewer --goods [--size WxH] [--angle RADIANS] [--out DIRECTORY]
//   ship_viewer --asteroids COLUMNS [--seed N] [--size WxH] [--angle RADIANS] [--out DIRECTORY]
//   ship_viewer --showcase SECONDS [--size WxH] [--out DIRECTORY]
//
// With --ship the named ship (0 to 35) is drawn; with --ship all each of the 36 ships is. With --obj a Wavefront OBJ
// file is drawn to obj.png instead (see open_elite/obj.h for the axes). With --asteroids a sheet of generated
// asteroids is drawn to asteroids.png: a row for each size class (splinter to giant), COLUMNS rocks across, starting
// from seed N. --showcase writes the
// pictures of the showcase at 0, 1, 2 ... SECONDS.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "open_elite/asteroids.h"
#include "open_elite/commodity_models.h"
#include "open_elite/equipment_models.h"
#include "open_elite/obj.h"
#include "open_elite/png.h"
#include "open_elite/renderer.h"
#include "open_elite/ships.h"
#include "open_elite/showcase.h"

namespace {

void Usage() {
  std::fprintf(stderr,
               "usage: ship_viewer [--ship N|all] [--size WxH] [--angle RADIANS] [--out DIRECTORY] [--no-stars]\n"
               "       ship_viewer --obj FILE.obj [--keep-z] [--flip-winding] [--sway] [--size WxH] [--angle RADIANS]\n"
               "                   [--out DIRECTORY]\n"
               "       ship_viewer --items [--size WxH] [--angle RADIANS] [--out DIRECTORY]\n"
               "       ship_viewer --goods [--size WxH] [--angle RADIANS] [--out DIRECTORY]\n"
               "       ship_viewer --asteroids COLUMNS [--seed N] [--size WxH] [--angle RADIANS] [--out DIRECTORY]\n"
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
  std::string obj_path;
  int asteroid_columns = 0;
  bool items = false;
  bool goods = false;
  std::uint32_t asteroid_seed = 1;
  open_elite::ObjOptions obj_options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const bool has_value = i + 1 < argc;
    if (arg == "--items") {
      items = true;
    } else if (arg == "--goods") {
      goods = true;
    } else if (arg == "--asteroids" && has_value) {
      asteroid_columns = std::atoi(argv[++i]);
      if (asteroid_columns < 1 || asteroid_columns > 32) {
        Usage();
        return 2;
      }
    } else if (arg == "--seed" && has_value) {
      asteroid_seed = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
    } else if (arg == "--obj" && has_value) {
      obj_path = argv[++i];
    } else if (arg == "--keep-z") {
      obj_options.flip_z = false;
    } else if (arg == "--flip-winding") {
      obj_options.flip_winding = true;
    } else if (arg == "--sway") {
      obj_options.spin = open_elite::Spin::kSway;
    } else if (arg == "--ship" && has_value) {
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
  if (goods) {
    // The trade goods, one picture each
    for (std::size_t index = 0; index < kCommodityCount; ++index) {
      const Model& model = AllCommodityModels()[index];
      Image image(width, height);
      if (stars) {
        DrawStars(image, 0);
      }
      renderer.DrawShip(image, model, angle);
      DrawCaption(image, model.name);
      char name[32];
      std::snprintf(name, sizeof name, "/good_%02zu.png", index);
      const bool ok = WritePng(image, out + name);
      std::printf("%s %s\n", ok ? "wrote" : "FAILED", (out + name).c_str());
      failures += ok ? 0 : 1;
    }
    return failures == 0 ? 0 : 1;
  }
  if (items) {
    // The things for sale, one picture each, named after the item
    for (std::size_t index = 0; index < kItemCount; ++index) {
      const Model& model = AllItemModels()[index];
      Image image(width, height);
      if (stars) {
        DrawStars(image, 0);
      }
      renderer.DrawShip(image, model, angle);
      DrawCaption(image, model.name);
      char name[32];
      std::snprintf(name, sizeof name, "/item_%02zu.png", index);
      const bool ok = WritePng(image, out + name);
      std::printf("%s %s\n", ok ? "wrote" : "FAILED", (out + name).c_str());
      failures += ok ? 0 : 1;
    }
    return failures == 0 ? 0 : 1;
  }
  if (asteroid_columns > 0) {
    // A sheet: a row for each size class, a different rock in each column. Each rock is drawn in a tile of its own.
    const std::uint32_t tile_w = std::max<std::uint32_t>(1, width / static_cast<std::uint32_t>(asteroid_columns));
    const std::uint32_t tile_h = std::max<std::uint32_t>(1, height / kRockClassCount);
    Image sheet(tile_w * static_cast<std::uint32_t>(asteroid_columns), tile_h * kRockClassCount);
    for (int row = 0; row < kRockClassCount; ++row) {
      for (int column = 0; column < asteroid_columns; ++column) {
        const AsteroidParams params = MakeAsteroidParams(
            asteroid_seed + static_cast<std::uint32_t>(column), static_cast<RockClass>(row));
        Image tile(tile_w, tile_h);
        if (stars) {
          DrawStars(tile, static_cast<std::uint32_t>(row * 31 + column));
        }
        renderer.DrawShip(tile, GenerateAsteroidModel(params), angle);
        for (std::uint32_t y = 0; y < tile_h; ++y) {
          std::memcpy(sheet.data() + (static_cast<std::size_t>(row) * tile_h + y) * sheet.width() * 3 +
                          static_cast<std::size_t>(column) * tile_w * 3,
                      tile.data() + static_cast<std::size_t>(y) * tile_w * 3, static_cast<std::size_t>(tile_w) * 3);
        }
      }
    }
    const std::string file = out + "/asteroids.png";
    const bool ok = WritePng(sheet, file);
    std::printf("%s %s\n", ok ? "wrote" : "FAILED", file.c_str());
    return ok ? 0 : 1;
  }
  if (!obj_path.empty()) {
    Model model;
    std::string error;
    if (!LoadObj(obj_path, obj_options, &model, &error)) {
      std::fprintf(stderr, "ship_viewer: %s\n", error.c_str());
      return 1;
    }
    Image image(width, height);
    if (stars) {
      DrawStars(image, 0);
    }
    renderer.DrawShip(image, model, angle);
    DrawCaption(image, model.name);
    const std::string file = out + "/obj.png";
    const bool ok = WritePng(image, file);
    std::printf("%s %s\n", ok ? "wrote" : "FAILED", file.c_str());
    return ok ? 0 : 1;
  }
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
