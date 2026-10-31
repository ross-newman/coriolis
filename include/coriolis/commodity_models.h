#ifndef CORIOLIS_COMMODITY_MODELS_H_
#define CORIOLIS_COMMODITY_MODELS_H_

#include <vector>

#include "coriolis/market.h"
#include "coriolis/model.h"

namespace coriolis {

/// The 3D model of a trade good, for the market screen: a crate of food, rolls of cloth, a hazard-striped barrel, a cage,
/// a bottle, a gift box, a pill bottle, a monitor, a gear, a bundle of rods, a gun, a pelt, crystals, a stack of gold
/// bars, platinum bars, a cut gem and a glowing alien relic. In the style of the ships: flat shaded and outlined, centred
/// and scaled to the same size.
const Model& GetCommodityModel(Commodity item);

/// All the goods, in Commodity order
const std::vector<Model>& AllCommodityModels();

}  // namespace coriolis

#endif  // CORIOLIS_COMMODITY_MODELS_H_
