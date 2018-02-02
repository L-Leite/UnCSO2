#pragma once

#define CSO2_COMPRESSED_VTF_HWORD_SIGNATURE 'C'
#define CSO2_COMPRESSED_VTF_LWORD_SIGNATURE (('O')|('2'<<8))

#pragma pack(push, 1)

typedef struct CSO2CompressedVtf_s
{
	struct
	{
		uint8_t iHighByte;
		uint16_t iLowWord;
	} Signature;
	uint8_t iProbs;
	uint32_t iOriginalSize;
	uint32_t iChunkSizes[];
} CSO2CompressedVtf_t;

#pragma pack(pop)

bool IsCompressedVtf( uint8_t* pVtfBuffer );
bool DecompressVtf( uint8_t*& pBuffer, uint32_t& iBufferSize );
