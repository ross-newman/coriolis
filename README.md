# open-elite

The start of an open source version of the space trading game Elite.

At the moment this is the **ship rendering engine**: solid, flat shaded 3D models of the 13 ships of the original
game's ship list, drawn by a small software rasteriser into an RGB picture. It was extracted from the Elite test
card of the [MediaX](https://github.com/Astute-Systems/MediaX) video library. Everything else (flight, universe,
trading, user interface) is still to be planned.

![The 13 ships](docs/ships.png)

## What is here

| Part | Where | What it does |
| --- | --- | --- |
| Ship models | `src/ships.cpp` | The 13 ships, built from vertices and polygons with `ModelBuilder` |
| Model builder | `include/open_elite/model_builder.h` | Helpers for hulls, rings, bands, fans, boxes; recentres and scales a ship |
| Renderer | `src/renderer.cpp` | Perspective projection, depth buffer, flat shading with a highlight, outlined edges, supersampling |
| Star field | `src/stars.cpp` | Three layers of stars drifting at different speeds, twinkling |
| Text | `src/text.cpp` | A small 5x7 capital letter font for ship names |
| Showcase | `src/showcase.cpp` | Each ship in turn (30 seconds each by default), driven by elapsed time |
| PNG writer | `src/png.cpp` | Writes a picture as an uncompressed PNG, so there are no dependencies |
| Ship viewer | `apps/ship_viewer` | Draws the ships to PNG files |

The library has no dependencies beyond a C++17 compiler and CMake.

## Build

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

## Ship viewer

```bash
./build/ship_viewer --ship all --size 1280x720 --out /tmp/ships   # every ship
./build/ship_viewer --ship 4 --angle 2.36 --out .                 # the Cobra Mk III bounty hunter
./build/ship_viewer --showcase 90 --size 640x360 --out /tmp/show  # a picture a second for 90 seconds
```

## Using the library

```cpp
#include "open_elite/png.h"
#include "open_elite/renderer.h"
#include "open_elite/ships.h"

using namespace open_elite;

Image image(1280, 720);
DrawStars(image, /*frame=*/0);
Renderer().DrawShip(image, GetShip(ShipId::kCobraMk3Trader), /*angle=*/1.0f);
DrawCaption(image, "COBRA MK III TRADER");
WritePng(image, "cobra.png");
```

`Showcase` does the same for a whole show: `Showcase().Render(image, elapsed_seconds)` draws the ship that is on
show after `elapsed_seconds`, turned and with the stars drifted to match. It takes the time as an argument rather
than reading the clock, so the same time always gives the same picture.

## The ships

In the order of the ship list of the original game. The two Cobra Mk III entries share a model and differ only in
paint.

| # | Ship | Notes |
| --- | --- | --- |
| 0 | Sidewinder | Small kite shaped fighter |
| 1 | Viper | Police interceptor |
| 2 | Mamba | Long, slim, three engines |
| 3 | Python | Heavy freighter |
| 4 | Cobra Mk III (bounty hunter) | Modelled from a three view silhouette |
| 5 | Thargoid | Octagonal alien ship |
| 6 | Cobra Mk III (trader) | Same model, different paint |
| 7 | Coriolis station | A cuboctahedron with a docking slot, rolls about its own axis |
| 8 | Missile | |
| 9 | Asteroid | A roughened icosphere |
| 10 | Cargo canister | |
| 11 | Thargon | |
| 12 | Escape pod | |

## Models and the original game

Elite was written by Ian Bell and David Braben, and the game, its name and its data are theirs. **This project does
not contain any code, data or artwork from the original game.** The ship models are original geometry. The Cobra
Mk III follows the proportions of a three view silhouette in the Elite Wiki (wiki.alioth.net); the other ships are
interpretations of each ship's general shape.

Please keep it that way: do not contribute anything taken from the original game or its source, including the
ship blueprint tables.

## Licence

Not chosen yet.

## Ideas to plan

None of this is started.

- A game loop and a flight model
- Universe and market generation
- A vector style cockpit view and HUD
- Input, sound and saving
- More ships, and ship data (speed, shields, weapons) kept apart from the shape
