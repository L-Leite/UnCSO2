#pragma once

#define DBG_PRINTF(message, ...) printf( "%s: " message, __func__, ##__VA_ARGS__ )
#define DBG_WPRINTF(message, ...) wprintf( L"%S: " message, __func__, ##__VA_ARGS__  );

#include <Windows.h>

#include <assert.h>
#include <intrin.h>	
#include <sstream>
#include <stdint.h>
#include <stdio.h>	  
#include <string>
#include <vector>
#include "vc17_filesystem.h"

#include "version.h"

extern std::filesystem::path g_PkgDataPath;
extern std::filesystem::path g_OutPath;
