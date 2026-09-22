#include "ben_debug.h"

std::ofstream debug;

bool initializeDebugLog(const std::string& path) {
    debug.open(path, std::ios::out | std::ios::trunc);
    return debug.is_open();
}
