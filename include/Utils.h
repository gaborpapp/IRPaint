#pragma once

#include <string>
#include <vector>

#include "cinder/gl/Texture.h"
#include "cinder/Filesystem.h"

namespace mndl {

//! Returns timestamp for current time.
std::string getTimestamp();

std::vector< ci::gl::Texture > loadTextures( const ci::fs::path &relativeDir );

} // namespace mndl
