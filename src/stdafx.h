#pragma once

#define DBG_PRINTF(message, ...) printf( "%s: " message, __func__, ##__VA_ARGS__ )
#define DBG_WPRINTF(message, ...) wprintf( L"%S: " message, __func__, ##__VA_ARGS__  );

#include <stdint.h>

#include <assert.h>
#include <intrin.h>	  
#include <stdio.h>	 

#include <filesystem>
#include <sstream>

#include <array>
#include <string>
#include <vector>

#include "version.h"

#ifdef _WIN32
#include <Windows.h>
#endif

extern std::filesystem::path g_PkgDataPath;
extern std::filesystem::path g_OutPath;
