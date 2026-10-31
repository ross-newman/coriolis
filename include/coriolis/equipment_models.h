#ifndef CORIOLIS_EQUIPMENT_MODELS_H_
#define CORIOLIS_EQUIPMENT_MODELS_H_

#include <cstddef>
#include <vector>

#include "coriolis/commander.h"
#include "coriolis/model.h"

namespace coriolis {

/// The number of things for sale: every Item
constexpr std::size_t kItemCount = 11;

/// The 3D model of a thing for sale, in the style of the ships: flat shaded and outlined, nose along +z, centred and
/// scaled to the same size. The missile and the escape pod are the ship models of the same names; the fuel tank, E.C.M.
/// unit, fuel scoop, energy bomb, docking computer and the beam and military lasers are made for the equipment screen.
const Model& GetItemModel(Item item);

/// All the item models, in Item order
const std::vector<Model>& AllItemModels();

}  // namespace coriolis

#endif  // CORIOLIS_EQUIPMENT_MODELS_H_
