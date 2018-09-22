#include <stdafx.h>	

#include <map>

#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/md5.h>
#include <cryptopp/modes.h>

#include "pkgindex.h"
#include "pkgfilesystem.h"
#include "decryption.h"

std::vector<std::string> g_vPkgFilenames;
std::string g_szPkgListFilename = CSO2_PKGLIST_FILENAME;

void FillPkgArrayFromBuffer( std::vector<std::string>& vOutPkgFIleNames, uint8_t* pFileBuffer )
{	
	static const char* szNewLine = "\r\n";
	static const size_t iNewLineLength = strlen( "\r\n" ); 

	std::string szPkgBuffer = (char*) pFileBuffer;		  
	size_t iCurrentPos = 0;
	size_t iOffset = 0;

	while ( (iOffset = szPkgBuffer.find( szNewLine, iCurrentPos )) != std::wstring::npos )
	{
		std::string szPkgEntry = szPkgBuffer.substr( iCurrentPos, iOffset - iCurrentPos );

		if ( szPkgEntry.find( ".pkg" ) != std::string::npos && szPkgEntry != g_szPkgListFilename )
			vOutPkgFIleNames.push_back( szPkgEntry );

		iCurrentPos = iOffset + iNewLineLength;
	}
}

bool GetPkgFileNames( const std::filesystem::path& pkgDirectoryPath, std::vector<std::string>& vOutPkgFileNames )
{						   
	std::filesystem::path szFullPkgListPath = pkgDirectoryPath.string() + g_szPkgListFilename;

	HANDLE hFile = CreateFileW( szFullPkgListPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, NULL, nullptr );

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		wprintf( L"GetPkgFileNames: Couldn't open %s! Last error: %i\n", szFullPkgListPath.c_str(), GetLastError() );
		return false;
	}

	CSO2PkgListHeader_t pkgListHeader;	 

	DWORD dwBytesRead = NULL;
	BOOL bResult = ReadFile( hFile, &pkgListHeader, sizeof( CSO2PkgListHeader_t ), &dwBytesRead, nullptr );

	if ( !bResult || dwBytesRead != sizeof( CSO2PkgListHeader_t ) )
	{
		printf( "GetPkgFileNames: Failed to read PkgListHeader! Last error: %i Bytes read: %u\n", GetLastError(), dwBytesRead );
		CloseHandle( hFile );
		return false;
	}

	uint8_t* pIndexFileBuffer = new uint8_t[pkgListHeader.iFileSize];

	bResult = ReadFile( hFile, pIndexFileBuffer, pkgListHeader.iFileSize, &dwBytesRead, nullptr );

	if ( !bResult || dwBytesRead != pkgListHeader.iFileSize )
	{
		printf( "GetPkgFileNames: Failed to read PkgList! Last error: %i Bytes read: %u\n", GetLastError(), dwBytesRead );
		CloseHandle( hFile );
		return false;
	}

	CloseHandle( hFile );

	if ( pkgListHeader.iVersion == CSO2_PKG_VERSION )
	{
		uint8_t digestedKey[CryptoPP::Weak::MD5::DIGESTSIZE];

		if ( !GeneratePkgListKey( pkgListHeader.iKey, g_szPkgListFilename.c_str(), digestedKey ) )
		{
			printf( "GetPkgFileNames: Failed to generate package list key! Key: %i PkgListFilename: %s\n", pkgListHeader.iKey, g_szPkgListFilename.c_str() );
			delete[] pIndexFileBuffer;
			return false;
		}

		if ( !DecryptBuffer( pkgListHeader.iCipher, pIndexFileBuffer, pkgListHeader.iFileSize, digestedKey, sizeof( digestedKey ) ) )
		{
			printf( "GetPkgFileNames: Failed to decrypt buffer! Cipher: %s (%i)\n", PkgCipherToString( pkgListHeader.iCipher ), pkgListHeader.iCipher );
			delete[] pIndexFileBuffer;
			return false;
		}
	}
	else
	{
		printf( "GetPkgFileNames: Package list file version is different from %i! (got %i)\n", CSO2_PKG_VERSION, pkgListHeader.iVersion );
		delete[] pIndexFileBuffer;
		return false;
	}	

	DetectGameDataProvider( pIndexFileBuffer );
	FillPkgArrayFromBuffer(vOutPkgFileNames, pIndexFileBuffer);	
	DBG_WPRINTF(L"Detected game data provider: %s\n", GetGameDataProviderStr());

	delete[] pIndexFileBuffer;					
	return true;
}
