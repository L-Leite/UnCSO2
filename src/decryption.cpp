#include <stdafx.h>
#include "decryption.h"

#include <cryptopp/aes.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/des.h>
#include <cryptopp/md5.h>
#include <cryptopp/modes.h>
#include "Rijndael.h"

static uint8_t s_PackageListKey[4][16] =
{
	{ 0x9A, 0xA6, 0xC7, 0x59, 0x18, 0xEA, 0xD0, 0x44, 0x83, 0xA3, 0x3A, 0x3E, 0xCE, 0xAF, 0x6F, 0x68 },
	{ 0xB6, 0xBA, 0x15, 0xC7, 0x77, 0x9D, 0x9C, 0x49, 0x84, 0x62, 0x2A, 0x9A, 0x8A, 0x61, 0x84, 0xA6 },
	{ 0x68, 0x55, 0x24, 0x24, 0x2B, 0xCB, 0x88, 0x4B, 0xA7, 0xA6, 0xD2, 0xC7, 0x94, 0xED, 0xE8, 0xD3 },
	{ 0x36, 0x24, 0xD6, 0x8C, 0x6C, 0xB8, 0xE1, 0x4A, 0xB1, 0x82, 0xC0, 0xA3, 0xDC, 0xE4, 0x16, 0xC8 }
};

static uint8_t s_EmptyIv16[16] = { 0 };

const char* PkgCipherToString( int iCipher )
{
	switch ( iCipher )
	{
		case PKGCIPHER_DES:
			return "PKGCIPHER_DES";

		case PKGCIPHER_AES:
			return "PKGCIPHER_AES";

		case PKGCIPHER_BLOWFISH:
			return "PKGCIPHER_BLOWFISH";

		case PKGCIPHER_RIJNDAEL:
			return "PKGCIPHER_RIJNDAEL";

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


// DES and Blowfish are both untested since they're not used
bool DecryptBuffer( int iCipher, uint8_t* pInBuffer, uint8_t* pOutBuffer, size_t iBufferSize, uint8_t* pKey, size_t iKeyLength, uint8_t* pIV /*= nullptr*/, size_t iIVLength /*= NULL*/ )
{		
	if ( !pInBuffer )
		throw std::exception( "DecryptBuffer: pInBuffer is NULL!\n" );

	if ( !iBufferSize )
		throw std::exception( "DecryptBuffer: iBufferSize is NULL!\n" );

	if ( !pIV )
	{
		pIV = s_EmptyIv16;
		iIVLength = sizeof( s_EmptyIv16 );
	}

	if ( iCipher == PKGCIPHER_DES )
	{
		iKeyLength = 8;
		iIVLength = 8;

		CryptoPP::CBC_Mode<CryptoPP::DES>::Decryption desDecrypt;
		desDecrypt.SetKeyWithIV( pKey, iKeyLength, pIV, iIVLength );
		desDecrypt.ProcessData( pOutBuffer, pInBuffer, iBufferSize );
	}
	else if ( iCipher == PKGCIPHER_AES )
	{	  				  
		CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption rijndaelDecrypt;	  
		rijndaelDecrypt.SetKeyWithIV( pKey, iKeyLength, pIV, iIVLength );
		rijndaelDecrypt.ProcessData( pOutBuffer, pInBuffer, iBufferSize );
	}
	else if ( iCipher == PKGCIPHER_BLOWFISH )
	{
		CryptoPP::Blowfish::Decryption bf( pKey, iKeyLength );

		for ( size_t i = 0; i < iBufferSize / bf.BlockSize(); i++ )
		{
			size_t offset = i * bf.BlockSize();
			bf.ProcessBlock( pInBuffer + offset, pOutBuffer + offset );
		}
	}	 
	else if ( iCipher == PKGCIPHER_RIJNDAEL )
	{
		// crypto++'s AES doesn't work with the packages,
		// they probably are encrypted with an old version of openssl	 

		bool bUsingTempBuffer = false;

		if ( pInBuffer == pOutBuffer )
		{
			bUsingTempBuffer = true;
			pOutBuffer = new uint8_t[iBufferSize];
		}											

		CRijndael rijndael;
		rijndael.MakeKey( (char*) pKey, (char*) pIV );
		rijndael.Decrypt( (char*) pInBuffer, (char*) pOutBuffer, iBufferSize, CRijndael::CBC );

		if ( bUsingTempBuffer )
		{	
			memcpy_s( pInBuffer, iBufferSize, pOutBuffer, iBufferSize );	
			delete[] pOutBuffer;
		}	  		
	}
	else
	{
		throw std::exception( "DecryptBuffer: Unknown cipher used %d", iCipher );
	}

	return true;
}
