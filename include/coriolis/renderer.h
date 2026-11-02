#ifndef CORIOLIS_RENDERER_H_
#define CORIOLIS_RENDERER_H_

#include <cstdint>
#include <string>

#include "coriolis/model.h"
#include "coriolis/types.h"

namespace coriolis {

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
  /// with Spin::kRoll rolls about its own long axis instead, showing its front to the viewer; one with Spin::kSway
  /// faces the viewer and swings from side to side.
  void DrawShip(Image& image, const Model& model, float angle) const;

 private:
  RenderOptions options_;
};

/// Paint a star field in three layers that drift at different speeds for parallax. `frame` counts frames; the same
/// frame always gives the same stars. The stars are added to what is in the image (brighter pixels win).
void DrawStars(Image& image, std::uint32_t frame);

/// Draw text in a vector stroke font at pixel position (x, y). The font is drawn as smooth anti-aliased lines, so it is
/// clean at any size: a character cell is 6*scale pixels wide and its capitals are 7*scale high. It has capitals,
/// lower case, digits and the usual punctuation; any other character is drawn as a space. The scale need not be a
/// whole number, so text can be sized to the picture exactly.
void DrawText(Image& image, const std::string& text, std::int64_t x, std::int64_t y, float scale, Rgb colour);

/// The text scale for the screens and menus of a picture this size: the same share of the picture at any resolution.
/// A screen is laid out as 240 units high and 400 wide (a row of text is 10 units), so the scale follows whichever of
/// the height and the width is the tighter fit. Never below 1.
float UiScale(const Image& image);

/// Smooth anti-aliased lines, circles and discs in the same weight as the text, so graphics and text sit together: a
/// line or ring at `scale` is as thick as the strokes of text at that scale. Positions are in pixels, and anything off
/// the picture is clipped.
void DrawLine(Image& image, float x0, float y0, float x1, float y1, float scale, Rgb colour);
void DrawCircle(Image& image, float cx, float cy, float radius, float scale, Rgb colour);
void DrawDisc(Image& image, float cx, float cy, float radius, Rgb colour);

/// Draw text in the bottom left corner, sized to the height of the image
void DrawCaption(Image& image, const std::string& text, Rgb colour = {120, 255, 160});

}  // namespace coriolis

#endif  // CORIOLIS_RENDERER_H_
