#include "stdafx.h"
#include "cso2vtf.h"	
#include "lzmaDecoder.h"

bool IsCompressedVtf( uint8_t* pVtfBuffer )
{
	if ( !pVtfBuffer )
		return false;

	CSO2CompressedVtf_t* pVtf = (CSO2CompressedVtf_t*) pVtfBuffer;

	return (pVtf->Signature.iHighByte == CSO2_COMPRESSED_VTF_HWORD_SIGNATURE
		&& pVtf->Signature.iLowWord == CSO2_COMPRESSED_VTF_LWORD_SIGNATURE);
}

bool DecompressVtf( uint8_t*& pBuffer, uint32_t& iBufferSize )
{
	if ( !iBufferSize )
	{
		DBG_PRINTF( "iBufferSize is null, pretending it's fine\n" );
		return true;
	}

	CSO2CompressedVtf_t* pVtf = (CSO2CompressedVtf_t*) pBuffer;

	if ( !pVtf->iOriginalSize )
	{	 
		DBG_PRINTF( "VTF's original size is null!\n" );
		return false;
	}

	uint8_t* pNewBuffer = new uint8_t[pVtf->iOriginalSize];
	uint32_t iOutOffset = 0;

	CLZMA lzma;

	for ( uint8_t i = 0; i < pVtf->iProbs; i++ )
	{
		uint32_t iChunkSize = pVtf->iChunkSizes[i * 2];

		unsigned int outSize;

		if ( iChunkSize & 1 )
		{
			outSize = lzma.Uncompress( pBuffer + (iChunkSize >> 1), pNewBuffer + iOutOffset );
		}
		else
		{
			outSize = pVtf->iChunkSizes[i * 2 + 1];
			memcpy( pNewBuffer + iOutOffset, pBuffer + (iChunkSize >> 1), outSize );
		}

		iOutOffset += outSize;
	}

	iBufferSize = pVtf->iOriginalSize;
	delete pBuffer;
	pBuffer = pNewBuffer;

	return true;
}
