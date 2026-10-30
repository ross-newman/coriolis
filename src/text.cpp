#include <algorithm>
#include <array>

#include "open_elite/renderer.h"

namespace open_elite {

namespace {

/// A 5x7 glyph, one string of 5 characters per row ('#' is lit)
struct Glyph {
  char character;
  std::array<const char*, 7> rows;
};

/// The letters needed for the names of the ships
const std::array<Glyph, 22> kFont{{
    {' ', {".....", ".....", ".....", ".....", ".....", ".....", "....."}},
    {'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
    {'C', {".####", "#....", "#....", "#....", "#....", "#....", ".####"}},
    {'D', {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}},
    {'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
    {'G', {".####", "#....", "#....", "#.###", "#...#", "#...#", ".###."}},
    {'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'I', {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
    {'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
    {'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
    {'N', {"#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"}},
    {'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
    {'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
    {'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
    {'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
    {'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
    {'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#"}},
    {'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
}};

}  // namespace

void DrawText(Image& image, const std::string& text, std::int64_t x, std::int64_t y, std::uint32_t scale, Rgb colour) {
  scale = std::max<std::uint32_t>(1, scale);
  std::int64_t x0 = x;
  for (const char character : text) {
    // Unknown characters are drawn as a space
    const Glyph* glyph = &kFont[0];
    for (const Glyph& g : kFont) {
      if (g.character == character) {
        glyph = &g;
      }
    }
    for (std::uint32_t row = 0; row < 7; ++row) {
      for (std::uint32_t column = 0; column < 5; ++column) {
        if (glyph->rows[row][column] != '#') {
          continue;
        }
        for (std::uint32_t dy = 0; dy < scale; ++dy) {
          for (std::uint32_t dx = 0; dx < scale; ++dx) {
            image.Set(x0 + static_cast<std::int64_t>(column * scale + dx),
                      y + static_cast<std::int64_t>(row * scale + dy), colour);
          }
        }
      }
    }
    x0 += static_cast<std::int64_t>(6 * scale);
  }
}

void DrawCaption(Image& image, const std::string& text, Rgb colour) {
  const std::uint32_t scale = std::max<std::uint32_t>(1, image.height() / 180);
  const std::int64_t margin = 4 * scale;
  DrawText(image, text, margin, static_cast<std::int64_t>(image.height()) - 7 * scale - margin, scale, colour);
}

}  // namespace open_elite
