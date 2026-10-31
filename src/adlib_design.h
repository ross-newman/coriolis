#ifndef OPEN_ELITE_ADLIB_DESIGN_H_
#define OPEN_ELITE_ADLIB_DESIGN_H_

#include <cstdint>
#include <random>
#include <vector>

#include "open_elite/adlib_synth.h"

namespace open_elite {

/// The sound effects designed for this game (everything after the effects of the original AdLib synthesizer), made with
/// a two operator FM voice like the OPL2 chip's, filtered noise and a few layers each. See adlib_design.cpp.
std::vector<std::int16_t> DesignEffect(SoundEffect effect, double sample_rate, std::mt19937& rng);

/// The name of a designed effect, or "?" for anything else
const char* DesignName(SoundEffect effect);

}  // namespace open_elite

#endif  // OPEN_ELITE_ADLIB_DESIGN_H_
