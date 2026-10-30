#ifndef OPEN_ELITE_SHOWCASE_H_
#define OPEN_ELITE_SHOWCASE_H_

#include <cstddef>
#include <cstdint>

#include "open_elite/renderer.h"
#include "open_elite/types.h"

namespace open_elite {

/// How the showcase runs
struct ShowcaseOptions {
  /// Seconds each ship is shown
  double seconds_per_ship = 30.0;
  /// Show this ship only (a ShipId value), or -1 to show every ship in turn
  int fixed_ship = -1;
  /// Radians the ship turns each second
  float turn_rate = 0.9f;
  /// Frames per second that the stars drift at
  float star_rate = 30.0f;
};

/// Each ship in turn, spinning in front of a star field, with its name at the bottom. Driven by elapsed time, not
/// by the clock, so it is repeatable.
class Showcase {
 public:
  explicit Showcase(ShowcaseOptions options = {}, RenderOptions render_options = {})
      : options_(options), renderer_(render_options) {}

  /// The index of the ship on show after `elapsed_seconds`, wrapping round after the last ship
  std::size_t ShipAt(double elapsed_seconds) const;

  /// Draw the picture for `elapsed_seconds` after the start
  void Render(Image& image, double elapsed_seconds) const;

 private:
  ShowcaseOptions options_;
  Renderer renderer_;
};

}  // namespace open_elite

#endif  // OPEN_ELITE_SHOWCASE_H_
