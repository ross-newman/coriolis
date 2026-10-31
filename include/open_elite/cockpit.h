#ifndef OPEN_ELITE_COCKPIT_H_
#define OPEN_ELITE_COCKPIT_H_

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "open_elite/game.h"
#include "open_elite/model.h"
#include "open_elite/nebula.h"
#include "open_elite/types.h"

namespace open_elite {

/// Which way the pilot looks
enum class CockpitView { kFront, kRear, kLeft, kRight };

/// The flight screen: the pilot's view out of the ship, drawn by the software rasteriser into an image. The top four
/// fifths are the view and nothing else: the stars and dust streaming past, the planet and sun, every ship and station
/// as a solid shaded model with proper depth, laser bolts, explosions, target brackets with a lead marker, and a little
/// text in the vector font (the camera, the message, money and rank, warnings). There is no frame round the view. The
/// bottom fifth is a 3D console with three computer screens (ship systems, a 3D scanner, navigation) and a row of
/// warning lights, painted in the colours of the player's own ship. The screens are drawn as pictures and mapped onto
/// the console. The finish adds optional supersampled edges and a soft glow.
///
/// It only reads the game. Hand it what happened (`Notice`) so it can show explosions and flashes, tell it how much time
/// has passed (`Advance`), and draw.
class Cockpit {
 public:
  Cockpit() = default;

  void set_view(CockpitView view) { view_ = view; }
  CockpitView view() const { return view_; }

  /// Tell it what the game reported since the last call: events (explosions, hits on the player) and messages (for the
  /// message line)
  void Notice(const Game& game, const std::vector<Event>& events, const std::vector<std::string>& messages);

  /// Age the effects by this many seconds
  void Advance(double seconds);

  /// Draw the flight view. Nothing is drawn but a blank screen if the game is not in flight.
  void Draw(Image& image, const Game& game, std::uint32_t frame);

  /// Draw this many times bigger and shrink to the image, for smooth edges (1 to 4; default 1)
  void set_supersample(int factor) { supersample_ = factor; }
  int supersample() const { return supersample_; }

  /// Soft glow round the bright parts (default on)
  void set_bloom(bool on) { bloom_ = on; }

  /// Gas clouds on the sky, made from the seed of the system (default on)
  void set_nebula(bool on) { nebula_on_ = on; }

 private:
  struct Flash {
    Vec3d position;
    double age = 0.0;
    double size = 10.0;
    std::uint32_t seed = 0;
  };

  const Model& ModelFor(const Blueprint& blueprint);
  void Render(Image& image, const Game& game, std::uint32_t frame);

  CockpitView view_ = CockpitView::kFront;
  int supersample_ = 1;
  bool bloom_ = true;
  bool nebula_on_ = true;
  Vec3d sway_{0.0, 0.0, 0.0};
  std::unordered_map<const Blueprint*, Model> models_;
  std::optional<Nebula> nebula_;  // the gas clouds of the current system
  std::vector<Flash> flashes_;
  std::string message_;
  double message_age_ = 100.0;
  double shield_flash_ = 0.0;
  double hull_flash_ = 0.0;
};

}  // namespace open_elite

#endif  // OPEN_ELITE_COCKPIT_H_
