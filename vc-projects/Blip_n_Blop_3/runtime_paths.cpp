#include "runtime_paths.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

std::filesystem::path application_directory;
std::filesystem::path original_working_directory;
std::filesystem::path resource_root;
std::filesystem::path user_data_directory;
bool legacy_working_directory = false;

std::filesystem::path normalizeLogicalPath(const std::filesystem::path& path) {
    std::string normalized = path.generic_string();
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return std::filesystem::path(normalized);
}

}  // namespace

namespace RuntimePaths {

bool initialize(std::string& error) {
    std::error_code ec;
    original_working_directory = std::filesystem::current_path(ec);
    if (ec) {
        error = "Cannot determine the process working directory: " + ec.message();
        return false;
    }

    char* sdl_base_path = SDL_GetBasePath();
    if (sdl_base_path == nullptr) {
        error = "SDL_GetBasePath failed: " + std::string(SDL_GetError());
        return false;
    }

    application_directory = std::filesystem::path(sdl_base_path).lexically_normal();
    SDL_free(sdl_base_path);

    const char* user_data_override = std::getenv("BLIPBLOP_USER_DATA_DIR");
    if (user_data_override && *user_data_override) {
        user_data_directory =
            std::filesystem::absolute(user_data_override, ec).lexically_normal();
        if (ec) {
            error = "Cannot resolve BLIPBLOP_USER_DATA_DIR: " + ec.message();
            return false;
        }
        std::filesystem::create_directories(user_data_directory, ec);
        if (ec) {
            error = "Cannot create user data directory: " + ec.message();
            return false;
        }
    } else {
        char* sdl_pref_path = SDL_GetPrefPath("BlipBlopModern", "BlipnBlop");
        if (sdl_pref_path == nullptr) {
            error = "SDL_GetPrefPath failed: " + std::string(SDL_GetError());
            return false;
        }
        user_data_directory =
            std::filesystem::path(sdl_pref_path).lexically_normal();
        SDL_free(sdl_pref_path);
    }

    const auto application_data = application_directory / "data";
    const auto legacy_data = original_working_directory / "data";

    if (std::filesystem::is_directory(application_data, ec) && !ec) {
        resource_root = application_directory;
        legacy_working_directory = false;
    } else {
        ec.clear();
        if (!std::filesystem::is_directory(legacy_data, ec) || ec) {
            error = "Required data directory was not found. Executable-relative path: " +
                    application_data.string() + "; legacy working-directory path: " +
                    legacy_data.string();
            return false;
        }
        resource_root = original_working_directory;
        legacy_working_directory = true;
    }

    std::filesystem::current_path(resource_root, ec);
    if (ec) {
        error = "Cannot select runtime resource directory " + resource_root.string() +
                ": " + ec.message();
        return false;
    }

    return true;
}

const std::filesystem::path& applicationDirectory() { return application_directory; }

const std::filesystem::path& resourceRoot() { return resource_root; }

const std::filesystem::path& userDataDirectory() { return user_data_directory; }

bool usingLegacyWorkingDirectory() { return legacy_working_directory; }

std::filesystem::path resolve(const std::filesystem::path& logicalPath) {
    const auto normalized = normalizeLogicalPath(logicalPath);
    if (normalized.is_absolute()) return normalized.lexically_normal();

    std::error_code ec;
    const auto primary = (application_directory / normalized).lexically_normal();
    if (std::filesystem::exists(primary, ec) && !ec) return primary;

    ec.clear();
    const auto fallback = (original_working_directory / normalized).lexically_normal();
    if (std::filesystem::exists(fallback, ec) && !ec) return fallback;

    // Return the preferred executable-relative location for useful diagnostics
    // and for writable legacy files such as data/bb.cfg and data/bb.scr.
    return primary;
}

std::string resolveString(const std::string& logicalPath) {
    return resolve(logicalPath).string();
}

std::filesystem::path writablePath(const std::filesystem::path& filename,
                                   const std::filesystem::path& legacyPath) {
    const auto destination = (user_data_directory / filename).lexically_normal();
    std::error_code ec;
    if (!legacyPath.empty() && !std::filesystem::exists(destination, ec)) {
        ec.clear();
        if (std::filesystem::is_regular_file(legacyPath, ec) && !ec) {
            ec.clear();
            std::filesystem::copy_file(legacyPath, destination,
                                       std::filesystem::copy_options::none, ec);
        }
    }
    return destination;
}

bool commitTemporaryFile(const std::filesystem::path& temporary,
                         const std::filesystem::path& destination,
                         std::string& error) {
#ifdef _WIN32
    if (MoveFileExW(temporary.c_str(), destination.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return true;
    }
    error = "MoveFileEx failed with error " + std::to_string(GetLastError());
    return false;
#else
    std::error_code ec;
    std::filesystem::rename(temporary, destination, ec);
    if (!ec) return true;
    error = ec.message();
    return false;
#endif
}

}  // namespace RuntimePaths
