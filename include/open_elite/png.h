#ifndef OPEN_ELITE_PNG_H_
#define OPEN_ELITE_PNG_H_

#include <string>

#include "open_elite/types.h"

namespace open_elite {

/// Write an image as a PNG file (uncompressed, so no dependencies)
///
/// \return true if the file was written
bool WritePng(const Image& image, const std::string& path);

}  // namespace open_elite

#endif  // OPEN_ELITE_PNG_H_
