#ifndef CORIOLIS_SCREENS_H_
#define CORIOLIS_SCREENS_H_

#include "coriolis/game.h"
#include "coriolis/types.h"

namespace coriolis {

/// The screens of the station and the charts, drawn into an image as text and simple graphics. They only read the game;
/// the front end decides which to show and what is selected.

/// Commander status: name, rank, money, fuel, legal status, equipment and cargo
void DrawStatusScreen(Image& image, const Game& game);

/// The market: every good with its price, what is on sale and what is in the hold. `selected` is the highlighted row.
/// When the picture is wide enough the highlighted good is drawn in 3D to the right of the list, turned by `angle`.
void DrawMarketScreen(Image& image, const Game& game, int selected, float angle = 2.4f);

/// The equipment for sale at this tech level, with prices. `selected` is the highlighted row. Below the list the
/// highlighted item is drawn in 3D, turned by `angle` radians.
void DrawEquipmentScreen(Image& image, const Game& game, int selected, float angle = 2.4f);

/// The galactic chart: every system in the galaxy, the one the ship is at, the range of the fuel and the chosen
/// destination, with the data on the chosen system below
void DrawChartScreen(Image& image, const Game& game);

/// The data on one system: government, economy, tech level, population, species, size and distance
void DrawSystemScreen(Image& image, const Game& game, int system_index);

/// The number of rows in the equipment screen
int EquipmentCount();

/// The item on a row of the equipment screen
Item EquipmentItem(int row);

}  // namespace coriolis

#endif  // CORIOLIS_SCREENS_H_
