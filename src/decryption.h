#pragma once

enum
{
	PKGCIPHER_DES = 1,
	PKGCIPHER_AES,
	PKGCIPHER_BLOWFISH,
	PKGCIPHER_RIJNDAEL
};	   

bool DecryptBuffer( int iCipher, uint8_t* pInBuffer, uint8_t* pOutBuffer, size_t iBufferSize, uint8_t* pKey, size_t iKeyLength, uint8_t* pIV = nullptr, size_t iIVLength = NULL );

inline bool DecryptBuffer( int iCipher, void* pInBuffer, size_t iBufferSize, uint8_t* pKey, size_t iKeyLength, uint8_t* pIV = nullptr, size_t iIVLength = NULL )
{
	return DecryptBuffer( iCipher, (uint8_t*) pInBuffer, (uint8_t*) pInBuffer, iBufferSize, pKey, iKeyLength, pIV, iIVLength );
}

template<size_t iBufferSize>
inline bool DecryptBuffer( int iCipher, uint8_t( &pInBuffer )[iBufferSize], uint8_t* pKey, size_t iKeyLength )
{
	return DecryptBuffer( iCipher, pInBuffer, pInBuffer, iBufferSize, pKey, iKeyLength );
}
