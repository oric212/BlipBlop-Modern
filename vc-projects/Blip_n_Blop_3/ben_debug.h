#pragma once

#include <fstream>
#include <string>

#ifndef DEBUG_CPP_FILE
extern std::ofstream debug;
#endif

bool initializeDebugLog(const std::string& path);
