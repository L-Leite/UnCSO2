#include <stdafx.h>
#include "decryption.h"

#include <cryptopp/aes.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/des.h>
#include <cryptopp/modes.h>
#include "Rijndael.h"

static uint8_t s_EmptyIv16[16] = { 0 };

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
		CryptoPP::DES_EDE2::Decryption des( pKey, iKeyLength );		

		for ( size_t i = 0; i < iBufferSize / des.BlockSize(); i++ )
		{
			size_t offset = i * des.BlockSize();
			des.ProcessBlock( pInBuffer + offset, pOutBuffer + offset );
		}
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
			delete pOutBuffer;
		}	  		
	}
	else
	{
		throw std::exception( "DecryptBuffer: Unknown cipher used %d", iCipher );
	}

	return true;
}
