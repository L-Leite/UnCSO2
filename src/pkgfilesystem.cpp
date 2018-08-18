#include "stdafx.h"
#include "decryption.h"
#include "pkgfilesystem.h"
#include "qt/pkgfilesystemmodel.h"

#include <cryptopp/md5.h>		

static const std::string s_szTiancityPackageFileKey = "\x9B\x65\xC7\x9B\xC7\xDF\x8E\x7E\xD4\xC6\x59\x52\x5C\xF7\x22\xFF\xF4\xE8\xFF\xE7\xB5\xC2\x77";
static const std::string s_szNexonPackageFileKey = "\x6C\x6B\x67\x75\x69\x37\x38\x31\x6B\x6C\x37\x38\x39\x73\x64\x21\x40\x23\x25\x38\x39\x26\x5E\x73\x64";

static const std::string s_szTiancityPackageEntryKey = "\x8E\x5C\xB8\x92\x45\xD1\x90\xBA\x82\x0F\xD9\x7A\x99\x8E\xB3\x87\xF7";
static const std::string s_szNexonPackageEntryKey = "\x5E\x39\x67\x45\x72\x67\x32\x53\x78\x37\x62\x6E\x6B\x37\x40\x23\x73\x64\x66\x6A\x6E\x68\x40";

GameDataProvider g_GameDataProvider = GameDataProvider::GAMEDATAPROVIDER_NONE;

static const std::string& GetPackageFileKey()
{
	switch (g_GameDataProvider)
	{
		case GAMEDATAPROVIDER_NEXON:
			return s_szNexonPackageFileKey;
		case GAMEDATAPROVIDER_TIANCITY:
			return s_szTiancityPackageFileKey;
		default:
			throw std::exception("Unknown game data provider!\n");
	}
}

static const std::string& GetPackageEntryKey()
{
	switch (g_GameDataProvider)
	{
		case GAMEDATAPROVIDER_NEXON:
			return s_szNexonPackageEntryKey;
		case GAMEDATAPROVIDER_TIANCITY:
			return s_szTiancityPackageEntryKey;
		default:
			throw std::exception("Unknown game data provider!\n");
	}
}

void ConvertPathToUnix( std::string& pathToConvert )
{
	for ( size_t iOffset = pathToConvert.find( '\\' ); iOffset != std::wstring::npos; iOffset = pathToConvert.find( '\\', iOffset ) )
		pathToConvert.replace( iOffset, 1, 1, '/' );
}

bool MakePkgKeyMd5( const std::string& szPkgName, const std::string& szKey, std::string& pOutKey )
{
	if ( szPkgName.empty() )
		return false;

	CryptoPP::Weak::MD5 md5;

	if ( !szKey.empty() )
		md5.Update( (uint8_t*) szKey.c_str(), szKey.length() );

	md5.Update( (uint8_t*) szPkgName.c_str(), szPkgName.length() );

	uint8_t digestedKey[CryptoPP::Weak::MD5::DIGESTSIZE];
	md5.Final( digestedKey );

	for ( uint8_t i = 0; i < sizeof( digestedKey ); i++ )
	{
		char buffer[4];
		sprintf_s( buffer, "%02x", digestedKey[i] );
		pOutKey.append( buffer );
	}

	return true;
}

bool LoadPkgEntries( const std::string& szPkgFilename, CPkgFileSystemModel* pFileSystemModel )
{
	std::filesystem::path szFullPkgPath = g_PkgDataPath.string() + szPkgFilename;

	HANDLE hPkgFile = CreateFileW( szFullPkgPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, NULL, nullptr );

	if ( hPkgFile == INVALID_HANDLE_VALUE )
	{
		DBG_WPRINTF( L"Couldn't open %s! LastError: %i\n", szFullPkgPath.c_str(), GetLastError() );
		return false;
	}

	DWORD dwCurrentFileOffset = 33;	// skips hash

	if ( SetFilePointer( hPkgFile, dwCurrentFileOffset, nullptr, FILE_BEGIN ) == INVALID_SET_FILE_POINTER )
	{
		DBG_PRINTF( "Failed to set %s's file pointer! LastError: %i\n", szPkgFilename.c_str(), GetLastError() );
		CloseHandle( hPkgFile );
		return false;
	}

	CSO2PkgHeader_t pkgHeader;

	DWORD dwBytesRead = NULL;
	BOOL bResult = ReadFile( hPkgFile, &pkgHeader, sizeof( pkgHeader ), &dwBytesRead, nullptr );

	if ( !bResult || dwBytesRead != sizeof( pkgHeader ) )
	{
		DBG_PRINTF( "Failed to read %s's pkgEntry! Result: %i Bytes read: %u LastError: %i\n", szPkgFilename.c_str(), bResult, dwBytesRead, GetLastError() );
		CloseHandle( hPkgFile );
		return false;
	}

	dwCurrentFileOffset += dwBytesRead;

	std::string szGeneratedKey;
	MakePkgKeyMd5( szPkgFilename, GetPackageFileKey(), szGeneratedKey );

	DecryptBuffer( PKGCIPHER_RIJNDAEL, &pkgHeader, sizeof( pkgHeader ), (uint8_t*) szGeneratedKey.c_str(), szGeneratedKey.length() );

	uint32_t dwFileStart = dwCurrentFileOffset + pkgHeader.iEntries * CSO2_PKGENTRYHEADER_SIZE;

	for ( int i = 0; i < pkgHeader.iEntries; i++ )
	{
		CSO2PkgFileEntryHeader_t fileEntryHeader;

		if ( SetFilePointer( hPkgFile, dwCurrentFileOffset, nullptr, FILE_BEGIN ) == INVALID_SET_FILE_POINTER )
		{
			DBG_PRINTF( "Failed to set %s's file pointer for %s header! LastError: %i\n", szPkgFilename.c_str(), pkgHeader.szDirectoryPath, GetLastError() );
			CloseHandle( hPkgFile );
			return false;
		}

		BOOL bResult = ReadFile( hPkgFile, &fileEntryHeader, sizeof( fileEntryHeader ), &dwBytesRead, nullptr );

		if ( !bResult || dwBytesRead != sizeof( fileEntryHeader ) )
		{
			DBG_PRINTF( "Failed to read %s %i fileEntryHeader! Result: %i Bytes read: %u LastError: %i\n", pkgHeader.szDirectoryPath, i, bResult, dwBytesRead, GetLastError() );
			CloseHandle( hPkgFile );
			return false;
		}

		dwCurrentFileOffset += dwBytesRead;

		DecryptBuffer( PKGCIPHER_RIJNDAEL, &fileEntryHeader, sizeof( fileEntryHeader ), (uint8_t*) szGeneratedKey.c_str(), szGeneratedKey.length() );

		std::string szFilePath = fileEntryHeader.szFilePath;
		ConvertPathToUnix( szFilePath );

		CCSO2PkgEntry* pPkgEntry = new CCSO2PkgEntry( szPkgFilename, szFilePath, dwFileStart + fileEntryHeader.iOffset, fileEntryHeader.iPackedSize, fileEntryHeader.iUnpackedSize, fileEntryHeader.bIsEncrypted );
		pFileSystemModel->CreateChild( pPkgEntry );
	}

	CloseHandle( hPkgFile );
	return true;
}

CCSO2PkgEntry::CCSO2PkgEntry( const std::string& szPkgFilename, const std::string& szEntryPath, uint32_t iFileOffset, uint32_t iPackedSize, uint32_t iUnpackedSize, bool bIsEntryEncrypted )
{
	m_szPkgFilename = szPkgFilename;
	m_szEntryPath = szEntryPath;
	m_iFileOffset = iFileOffset;
	m_iPackedSize = iPackedSize;
	m_iUnpackedSize = iUnpackedSize;
	m_bIsEntryEncrypted = bIsEntryEncrypted;
}

CCSO2PkgEntry::~CCSO2PkgEntry()
{
}

bool CCSO2PkgEntry::ReadPkgEntry( uint8_t** pOutBuffer, uint32_t* pOutSize /*= nullptr*/ )
{
	std::string szGeneratedKey;

	if ( !pOutBuffer )
	{
		DBG_PRINTF( "pOutBuffer is nullptr!\n" );
		return false;
	}

	if ( !m_iPackedSize )
		return true;

	std::filesystem::path szFullPkgPath = g_PkgDataPath.string() + m_szPkgFilename;

	HANDLE hPkgFile = CreateFileW( szFullPkgPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, NULL, nullptr );

	if ( hPkgFile == INVALID_HANDLE_VALUE )
	{
		DBG_WPRINTF( L"Couldn't open %S! LastError: %i\n", m_szPkgFilename.c_str(), GetLastError() );
		return false;
	}

	std::filesystem::path filePath = m_szEntryPath;

	if ( m_bIsEntryEncrypted )
		MakePkgKeyMd5( filePath.filename().string().c_str(), GetPackageEntryKey(), szGeneratedKey );

	assert( m_iPackedSize >= m_iUnpackedSize );	// Packed size should always be bigger than unpacked
	uint8_t* pBuffer = new uint8_t[m_iPackedSize];		 	

	DWORD dwBytesRead = 0;
	BOOL bResult = FALSE;

	for ( uint32_t i = 0; i < m_iPackedSize; i += 0x10000 )
	{					 
		if ( SetFilePointer( hPkgFile, m_iFileOffset + i, nullptr, NULL ) == INVALID_SET_FILE_POINTER )
		{
			DBG_PRINTF( "Failed to SetFilePointer for %s at %s! Last error: %i\n", m_szEntryPath.c_str(), m_szPkgFilename.c_str(), GetLastError() );
			return false;
		}

		if ( !(bResult = ReadFile( hPkgFile, pBuffer + i, m_iPackedSize - i, &dwBytesRead, nullptr ))
			|| (m_iPackedSize - i) - dwBytesRead > 16 )
		{
			DBG_PRINTF( "Failed to ReadFile for %s at %s! Last error: %i ReadFile result: %s Bytes read: 0x%X\n", m_szEntryPath.c_str(), m_szPkgFilename.c_str(), GetLastError(), bResult ? "TRUE" : "FALSE", dwBytesRead );
			return false;
		}

		if ( m_bIsEntryEncrypted )
			DecryptBuffer( PKGCIPHER_RIJNDAEL, pBuffer + i, m_iPackedSize - i, (uint8_t*) szGeneratedKey.c_str(), szGeneratedKey.length() );
	}

	*pOutBuffer = pBuffer;

	if ( pOutSize )
		*pOutSize = m_iUnpackedSize;

	return true;
}
