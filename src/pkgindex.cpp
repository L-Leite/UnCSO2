#include <stdafx.h>	

#include <map>

#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/md5.h>
#include <cryptopp/modes.h>

#include "pkgindex.h"
#include "decryption.h"

std::vector<std::string> g_vPkgFilenames;
std::string g_szPkgListFilename = CSO2_CHN_PKGLISTFILENAME;

static uint8_t s_PackageListKey[4][16] =
{
	{ 0x9A, 0xA6, 0xC7, 0x59, 0x18, 0xEA, 0xD0, 0x44, 0x83, 0xA3, 0x3A, 0x3E, 0xCE, 0xAF, 0x6F, 0x68 },
	{ 0xB6, 0xBA, 0x15, 0xC7, 0x77, 0x9D, 0x9C, 0x49, 0x84, 0x62, 0x2A, 0x9A, 0x8A, 0x61, 0x84, 0xA6 },
	{ 0x68, 0x55, 0x24, 0x24, 0x2B, 0xCB, 0x88, 0x4B, 0xA7, 0xA6, 0xD2, 0xC7, 0x94, 0xED, 0xE8, 0xD3 },
	{ 0x36, 0x24, 0xD6, 0x8C, 0x6C, 0xB8, 0xE1, 0x4A, 0xB1, 0x82, 0xC0, 0xA3, 0xDC, 0xE4, 0x16, 0xC8 }
};

static const char* PkgCipherToString( int iCipher )
{	
	switch ( iCipher )
	{
		case PKGCIPHER_DES:
			return "PKGCIPHER_DES";

		case PKGCIPHER_AES:
			return "PKGCIPHER_AES";

		case PKGCIPHER_BLOWFISH:
			return "PKGCIPHER_BLOWFISH";

		default:
			return "UNKNOWN";
	}
}

bool GeneratePkgListKey( int iKey, const char* szPackageIndexName, uint8_t* pOutKey )
{		  
	CryptoPP::Weak::MD5 md5;

	uint32_t iStartData = 2;

	md5.Update( (uint8_t*) &iStartData, sizeof( iStartData ) );

	if ( iKey % 2 )
	{
		md5.Update( s_PackageListKey[iKey / 2], 16 );

		size_t iStrLength = strlen( szPackageIndexName );

		if ( iStrLength )
			md5.Update( (uint8_t*) szPackageIndexName, iStrLength );
	}
	else
	{
		size_t iStrLength = strlen( szPackageIndexName );

		if ( iStrLength )
			md5.Update( (uint8_t*) szPackageIndexName, iStrLength );

		md5.Update( s_PackageListKey[iKey / 2], 16 );
	}

	md5.Final( pOutKey );
	return true;
}

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

	PkgListHeader_t pkgListHeader;	 

	DWORD dwBytesRead = NULL;
	BOOL bResult = ReadFile( hFile, &pkgListHeader, sizeof( PkgListHeader_t ), &dwBytesRead, nullptr );

	if ( !bResult || dwBytesRead != sizeof( PkgListHeader_t ) )
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

	FillPkgArrayFromBuffer( vOutPkgFileNames, pIndexFileBuffer );

	delete[] pIndexFileBuffer;					
	return true;
}
