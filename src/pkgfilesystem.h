#pragma once

#include <map>

#define CSO2_PKGHEADER_SIZE 272
#define CSO2_PKGENTRYHEADER_SIZE 288

#define CSO2_ENCRYPTED_FILE_VER 2

#pragma pack(push, 1)

typedef struct CSO2PkgHeader_s
{		
	char szDirectoryPath[261];
	uint32_t Unknown261;
	uint32_t iEntries;
	uint8_t Pad[3];
} CSO2PkgHeader_t;

static_assert(sizeof( CSO2PkgHeader_t ) == CSO2_PKGHEADER_SIZE, "CSO2PkgHeader_t size is different from CSO2_PKGHEADER_SIZE!");

typedef struct CSO2PkgFileEntryHeader_s
{						
	char szFilePath[MAX_PATH + 1];
	uint32_t iOffset;	  	
	uint32_t iPackedSize;
	uint32_t iUnpackedSize;
	uint8_t Unknown273;
	uint8_t bIsEncrypted;
	uint8_t Pad[13];
} CSO2PkgFileEntryHeader_t;

static_assert(sizeof( CSO2PkgFileEntryHeader_t ) == CSO2_PKGENTRYHEADER_SIZE, "CSO2PkgFileEntryHeader_t size is different from CSO2_PKGENTRYHEADER_SIZE!");

typedef struct CSO2EncFileHeader_s
{
	char szChecksum[10];
	uint16_t iVersion;
	uint8_t iEncryption;
	uint8_t iFlag;
	uint32_t iFileSize;
} CSO2EncFileHeader_t;

#pragma pack(pop)

class CPkgFileSystemModel;

class CCSO2PkgEntry
{
public:	  	
	CCSO2PkgEntry( const std::string& szPkgFilename, const std::string& szEntryPath, uint32_t iFileOffset, uint32_t iPackedSize, uint32_t iUnpackedSize, bool bIsEntryEncrypted );
	~CCSO2PkgEntry();

	bool ReadPkgEntry( uint8_t** pOutBuffer, uint32_t* pOutSize = nullptr );

	inline const std::string& GetEntryPath() { return m_szEntryPath;  }
	inline uint32_t GetFileOffset() { return m_iFileOffset; }
	inline uint32_t GetPackedSize() { return m_iPackedSize; }
	inline uint32_t GetUnpackedSize() { return m_iUnpackedSize; }

private:	
	std::string m_szPkgFilename;

	std::string m_szEntryPath;
	uint32_t m_iFileOffset;
	uint32_t m_iPackedSize;
	uint32_t m_iUnpackedSize;
	bool m_bIsEntryEncrypted;

private:
	CCSO2PkgEntry() = delete;
	CCSO2PkgEntry( const CCSO2PkgEntry& ) = delete;
	CCSO2PkgEntry& operator= ( const CCSO2PkgEntry& ) = delete;
};				

bool LoadPkgEntries( const std::string& szPkgFilename, CPkgFileSystemModel* pFileSystemModel );

enum GameDataProvider
{
	GAMEDATAPROVIDER_NONE = -1,
	GAMEDATAPROVIDER_NEXON = 0, // south korean version
	GAMEDATAPROVIDER_TIANCITY // chinese version
};

extern GameDataProvider g_GameDataProvider;

inline void DetectGameDataProvider( const uint8_t* pFileBuffer )
{			 
	std::string szPkgBuffer = reinterpret_cast<const char*>(pFileBuffer);
	bool bFoundTiancityDll = szPkgBuffer.find( "sedata.dll" ) != std::string::npos;

	if (bFoundTiancityDll)
		g_GameDataProvider = GAMEDATAPROVIDER_TIANCITY;
	else
		g_GameDataProvider = GAMEDATAPROVIDER_NEXON;
}

inline const wchar_t* GetGameDataProviderStr()
{
	switch (g_GameDataProvider)
	{
		case GAMEDATAPROVIDER_NEXON:
			return L"Nexon (South Korea)";
		case GAMEDATAPROVIDER_TIANCITY:
			return L"Tiancity (China)";
		default:
			return L"unknown";
	}
}
