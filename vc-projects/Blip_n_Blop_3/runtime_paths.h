#pragma once

#include <filesystem>
#include <string>

namespace RuntimePaths {

// Selects the executable directory as the runtime root when it contains data/.
// The original working directory is retained only as a development fallback.
bool initialize(std::string& error);

const std::filesystem::path& applicationDirectory();
const std::filesystem::path& resourceRoot();
const std::filesystem::path& userDataDirectory();
bool usingLegacyWorkingDirectory();

// Resolves a logical game path (for example, data/menu.lft). Backslashes in
// metadata are treated as separators without modifying the source data.
std::filesystem::path resolve(const std::filesystem::path& logicalPath);
std::string resolveString(const std::string& logicalPath);

// Returns a path in the SDL-managed per-user writable directory. On first use,
// an existing executable/source-tree legacy file can be copied there.
std::filesystem::path writablePath(const std::filesystem::path& filename,
                                   const std::filesystem::path& legacyPath = {});
bool commitTemporaryFile(const std::filesystem::path& temporary,
                         const std::filesystem::path& destination,
                         std::string& error);

}  // namespace RuntimePaths
