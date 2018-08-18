#pragma once

#include <stdint.h>

// make these overridable by the user soon
#define CSO2_PKG_VERSION 2
#define CSO2_PKGLIST_FILENAME "1b87c6b551e518d11114ee21b7645a47.pkg"

typedef struct CSO2PkgListHeader_s
{
	uint16_t iVersion;
	uint8_t iCipher;
	uint8_t iKey;
	uint32_t iFileSize;
} CSO2PkgListHeader_t;

bool GetPkgFileNames( const std::filesystem::path& pkgDirectoryPath, std::vector<std::string>& vOutPkgFileNames );
