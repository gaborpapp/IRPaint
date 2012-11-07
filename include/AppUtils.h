#pragma once

#include <string>

namespace mndl { namespace app {

//! Presents the user with a modal dialog and returns the option the user chose.
int showMessageBox( const std::string &message, const std::string &title );

} } // namespace mndl::app

