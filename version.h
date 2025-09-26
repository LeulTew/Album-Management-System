#pragma once

#include <string>

// Auto-generated fallback header for environments without CMake configure_file.
// Update the VERSION file to bump releases; this header should be regenerated manually
// when building without the CMake pipeline.

#define ALBUM_APP_VERSION "1.0.0"

inline std::string getAlbumAppVersion() {
    return ALBUM_APP_VERSION;
}
