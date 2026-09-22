#pragma once

#include <filesystem>
#include <string>

namespace RuntimePaths {

// Selects the executable directory as the runtime root when it contains data/.
// The original working directory is retained only as a development fallback.
bool initialize(std::string& error);

const std::filesystem::path& applicationDirectory();
const std::filesystem::path& resourceRoot();
bool usingLegacyWorkingDirectory();

// Resolves a logical game path (for example, data/menu.lft). Backslashes in
// metadata are treated as separators without modifying the source data.
std::filesystem::path resolve(const std::filesystem::path& logicalPath);
std::string resolveString(const std::string& logicalPath);

}  // namespace RuntimePaths
