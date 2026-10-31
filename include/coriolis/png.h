#ifndef CORIOLIS_PNG_H_
#define CORIOLIS_PNG_H_

#include <string>

#include "coriolis/types.h"

namespace coriolis {

/// Write an image as a PNG file (uncompressed, so no dependencies)
///
/// \return true if the file was written
bool WritePng(const Image& image, const std::string& path);

}  // namespace coriolis

#endif  // CORIOLIS_PNG_H_
