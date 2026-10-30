#ifndef OPEN_ELITE_RENDERER_H_
#define OPEN_ELITE_RENDERER_H_

#include <cstdint>
#include <string>

#include "open_elite/model.h"
#include "open_elite/types.h"

namespace open_elite {

/// How a ship is drawn
struct RenderOptions {
  /// Samples per pixel along each axis to smooth edges. 0 chooses 2 for pictures up to 1024x768 and 1 above.
  std::uint32_t supersample = 0;
  /// Distance from the camera to the middle of the ship, in ship radii (a ship is scaled to a radius of 3.2)
  float camera_distance = 11.5f;
  /// Focal length as a multiple of the shorter side of the picture
  float focal = 1.25f;
};

/// A software renderer for the ships: perspective projection, a depth buffer, flat shading with a specular
/// highlight, and outlined edges. It draws over what is already in the image.
class Renderer {
 public:
  explicit Renderer(RenderOptions options = {}) : options_(options) {}

  /// Draw a ship turned by `angle` radians. A ship spins about the vertical axis with a slow nod and roll; a ship
  /// with Spin::kRoll rolls about its own long axis instead, showing its front to the viewer.
  void DrawShip(Image& image, const Model& model, float angle) const;

 private:
  RenderOptions options_;
};

/// Paint a star field in three layers that drift at different speeds for parallax. `frame` counts frames; the same
/// frame always gives the same stars. The stars are added to what is in the image (brighter pixels win).
void DrawStars(Image& image, std::uint32_t frame);

/// Draw text in a 5x7 font at pixel position (x, y), each font pixel being `scale` pixels square. The font has the
/// capital letters used in ship names (A B C D E G H I K L M N O P R S T U V W Y) and the space; any other
/// character is drawn as a space.
void DrawText(Image& image, const std::string& text, std::int64_t x, std::int64_t y, std::uint32_t scale, Rgb colour);

/// Draw text in the bottom left corner, sized to the height of the image
void DrawCaption(Image& image, const std::string& text, Rgb colour = {120, 255, 160});

}  // namespace open_elite

#endif  // OPEN_ELITE_RENDERER_H_
