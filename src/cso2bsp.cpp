#include "stdafx.h"	   

#include <cryptopp/crc.h>
#include <map>

#include "cso2bsp.h"	
#include "cso2vtf.h"
#include "cso2zip_uncompressed.h"
#include "lzmaDecoder.h"

static char *s_LumpNames[] = {
	"LUMP_ENTITIES",						// 0
	"LUMP_PLANES",							// 1
	"LUMP_TEXDATA",							// 2
	"LUMP_VERTEXES",						// 3
	"LUMP_VISIBILITY",						// 4
	"LUMP_NODES",							// 5
	"LUMP_TEXINFO",							// 6
	"LUMP_FACES",							// 7
	"LUMP_LIGHTING",						// 8
	"LUMP_OCCLUSION",						// 9
	"LUMP_LEAFS",							// 10
	"LUMP_FACEIDS",							// 11
	"LUMP_EDGES",							// 12
	"LUMP_SURFEDGES",						// 13
	"LUMP_MODELS",							// 14
	"LUMP_WORLDLIGHTS",						// 15
	"LUMP_LEAFFACES",						// 16
	"LUMP_LEAFBRUSHES",						// 17
	"LUMP_BRUSHES",							// 18
	"LUMP_BRUSHSIDES",						// 19
	"LUMP_AREAS",							// 20
	"LUMP_AREAPORTALS",						// 21
	"LUMP_UNUSED0",							// 22
	"LUMP_UNUSED1",							// 23
	"LUMP_UNUSED2",							// 24
	"LUMP_UNUSED3",							// 25
	"LUMP_DISPINFO",						// 26
	"LUMP_ORIGINALFACES",					// 27
	"LUMP_PHYSDISP",						// 28
	"LUMP_PHYSCOLLIDE",						// 29
	"LUMP_VERTNORMALS",						// 30
	"LUMP_VERTNORMALINDICES",				// 31
	"LUMP_DISP_LIGHTMAP_ALPHAS",			// 32
	"LUMP_DISP_VERTS",						// 33
	"LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS",	// 34
	"LUMP_GAME_LUMP",						// 35
	"LUMP_LEAFWATERDATA",					// 36
	"LUMP_PRIMITIVES",						// 37
	"LUMP_PRIMVERTS",						// 38
	"LUMP_PRIMINDICES",						// 39
	"LUMP_PAKFILE",							// 40
	"LUMP_CLIPPORTALVERTS",					// 41
	"LUMP_CUBEMAPS",						// 42
	"LUMP_TEXDATA_STRING_DATA",				// 43
	"LUMP_TEXDATA_STRING_TABLE",			// 44
	"LUMP_OVERLAYS",						// 45
	"LUMP_LEAFMINDISTTOWATER",				// 46
	"LUMP_FACE_MACRO_TEXTURE_INFO",			// 47
	"LUMP_DISP_TRIS",						// 48
	"LUMP_PHYSCOLLIDESURFACE",				// 49
	"LUMP_WATEROVERLAYS",					// 50
	"LUMP_LEAF_AMBIENT_INDEX_HDR",			// 51
	"LUMP_LEAF_AMBIENT_INDEX",				// 52
	"LUMP_LIGHTING_HDR",					// 53
	"LUMP_WORLDLIGHTS_HDR",					// 54
	"LUMP_LEAF_AMBIENT_LIGHTING_HDR",		// 55
	"LUMP_LEAF_AMBIENT_LIGHTING",			// 56
	"LUMP_XZIPPAKFILE",						// 57
	"LUMP_FACES_HDR",						// 58
	"LUMP_MAP_FLAGS",						// 59
	"LUMP_OVERLAY_FADES",					// 60
};

class CLumpManager
{
public:
	CLumpManager( uint8_t* pBspBuffer )
	{
		SetBspBuffer( pBspBuffer );
		DecompressAndRebuildBsp();
	}

	~CLumpManager()
	{
		Cleanup();
	}

	template<typename T>
	inline T* GetLumpDataById( int iLumpId )
	{
		return (T*) (m_pBspBuffer + m_pBspHeader->lumps[iLumpId].fileofs);
	}

	inline int GetLumpOffsetById( int iLumpId )
	{
		return m_pBspHeader->lumps[iLumpId].fileofs;
	}

	inline int GetLumpSizeById( int iLumpId )
	{
		return m_pBspHeader->lumps[iLumpId].filelen;
	}

	inline int GetLumpVersionById( int iLumpId )
	{
		return m_pBspHeader->lumps[iLumpId].version;
	}

	inline std::vector<uint8_t>& GetNewLumpDataById( int iLumpId )
	{
		return m_NewLumpsData[iLumpId];
	}

	// avoid using this often
	inline int GetNewLumpOffsetById( int iLumpId )
	{
		int iOffset = sizeof( m_NewBspHeader );

		for ( int i = 0; i < iLumpId; i++ )
			iOffset += m_NewLumpsData[i].size();

		return iOffset;
	}

	inline size_t GetNewLumpTotalDataSize()
	{
		size_t iSize = 0;

		for ( int i = 0; i < HEADER_LUMPS; i++ )
			iSize += m_NewLumpsData[i].size();

		return iSize;
	}

	void BuildNewLumpsHeaders()
	{		 
		int iCurrentOffset = sizeof( m_NewBspHeader );

		for ( int i = 0; i < HEADER_LUMPS; i++ )
		{
			lump_t* pHeader = &m_NewBspHeader.lumps[i];
			int iLumpSize = m_NewLumpsData[i].size();

			pHeader->fileofs = iCurrentOffset;
			pHeader->filelen = iLumpSize;
			//pHeader->version = 0; // we already set the version in DecompressAndRebuildBsp 
			pHeader->uncompressedSize = 0;

			iCurrentOffset += iLumpSize;
		}
	}

	// when everything is done, allocate memory for the new bsp and copy contents to it
	bool CreateNewBspBuffer( uint8_t*& pOutBuffer, uint32_t& iOutBufferSize )
	{				
		size_t iTotalLumpSize = GetNewLumpTotalDataSize();		

		if ( !iTotalLumpSize )
		{
			DBG_PRINTF( "Total lump size is null!\n" );
			pOutBuffer = nullptr;
			iOutBufferSize = 0;
			return false;
		}

		size_t iBspHeaderSize = sizeof( m_NewBspHeader );
		size_t iNewBufferSize = iBspHeaderSize + iTotalLumpSize;

		uint8_t* pNewBspBuffer = new uint8_t[iNewBufferSize];
		memcpy( pNewBspBuffer, &m_NewBspHeader, iBspHeaderSize );

		uintptr_t iCurrentOffset = iBspHeaderSize;

		for ( int i = 0; i < HEADER_LUMPS; i++ )
		{
			std::vector<uint8_t>& vLumpData = m_NewLumpsData[i];

			size_t iLumpDataSize = vLumpData.size();				 
			memcpy( pNewBspBuffer + iCurrentOffset, vLumpData.data(), iLumpDataSize );

			iCurrentOffset += iLumpDataSize;
		}

		pOutBuffer = pNewBspBuffer;
		iOutBufferSize = iNewBufferSize;
		return true;
	}

private:
	inline void SetBspBuffer( uint8_t* pBspBuffer )
	{
		m_pBspBuffer = pBspBuffer;
		m_pBspHeader = (dheader_t*) m_pBspBuffer;
	}

	inline void Cleanup()
	{
		delete m_pBspBuffer;
	}

	void DecompressAndRebuildBsp()
	{
		std::vector<uint8_t> vBspBuffer;
		vBspBuffer.insert( vBspBuffer.end(), m_pBspBuffer, m_pBspBuffer + sizeof( dheader_t ) );

		CLZMA lzma;

		for ( uint8_t i = 0; i < HEADER_LUMPS; i++ )
		{
			lump_t* pLump = &m_pBspHeader->lumps[i];

			uint8_t* pTempBuffer = nullptr;
			unsigned int iActualSize = 0;

			if ( pLump->version & CSO2_LUMP_COMPRESSED )
			{
				iActualSize = lzma.GetActualSize( m_pBspBuffer + pLump->fileofs );

				if ( iActualSize )
				{
					pTempBuffer = new uint8_t[iActualSize];
					lzma.Uncompress( m_pBspBuffer + pLump->fileofs, pTempBuffer );
				}
				else
				{
					DBG_PRINTF( "Lump %s has compressed flagged but invalid lzma header?\n", s_LumpNames[i] );
					assert( 0 );
				}

				pLump->version &= ~CSO2_LUMP_COMPRESSED;	   				
			}
			else
			{
				iActualSize = pLump->filelen;
				pTempBuffer = new uint8_t[iActualSize];
				memcpy( pTempBuffer, m_pBspBuffer + pLump->fileofs, iActualSize );
			}

			pLump->fileofs = vBspBuffer.size();
			pLump->filelen = iActualSize;

			pLump->uncompressedSize = 0;

			m_NewBspHeader.lumps[i].version = pLump->version; // give the new headers the uncompressed version

			vBspBuffer.insert( vBspBuffer.end(), pTempBuffer, pTempBuffer + iActualSize );
			delete pTempBuffer;

			DBG_PRINTF( "lump: %s version: %i size: %i\n", s_LumpNames[i], pLump->version, pLump->filelen );
		}

		m_NewBspHeader.ident = IDBSPHEADER;
		m_NewBspHeader.version = 20;
		m_NewBspHeader.mapRevision = m_pBspHeader->mapRevision;

		memcpy( vBspBuffer.data(), m_pBspHeader, sizeof( dheader_t ) );

		delete m_pBspBuffer;

		uint8_t* pNewBuffer = new uint8_t[vBspBuffer.size()];
		memcpy( pNewBuffer, vBspBuffer.data(), vBspBuffer.size() );

		SetBspBuffer( pNewBuffer );
	}

private:
	dheader_t* m_pBspHeader;
	uint8_t* m_pBspBuffer;

	dheader_t m_NewBspHeader;
	std::vector<uint8_t> m_NewLumpsData[HEADER_LUMPS];
};

#define FIX_LUMP_FUNC( lump ) void Fix##lump( int iLumpId, CLumpManager* pLumpManager ); \
						CFixLumpFunc FixLumpFunc##lump( lump, &Fix##lump ); \
						void Fix##lump( int iLumpId, CLumpManager* pLumpManager )

#define FIX_LUMP_FUNC_EXISTING( lump, existingLump ) CFixLumpFunc FixLumpFunc##lump( lump, &Fix##existingLump );

#define FIX_LUMP_ID iLumpId
#define FIX_LUMP_MANAGER pLumpManager

class CFixLumpFunc
{
public:
	CFixLumpFunc( int iLumpId, void( *pFunction )(int, CLumpManager*) )
	{
		m_FunctionMap.emplace( iLumpId, pFunction );
	}
		
	// the default fix func, it copies the original lump content to the new lump
	static void DefaultFixLumpFunc( int iLump, CLumpManager* pLumpManager )
	{					
		uint8_t* pLumpData = pLumpManager->GetLumpDataById<uint8_t>( iLump );
		unsigned int iLumpSize = pLumpManager->GetLumpSizeById( iLump );
		std::vector<uint8_t>& vNewLump = pLumpManager->GetNewLumpDataById( iLump );		  

		if ( !iLumpSize )
			return;
		
		vNewLump.insert( vNewLump.end(),
			pLumpManager->GetLumpDataById<uint8_t>( iLump ),
			pLumpManager->GetLumpDataById<uint8_t>( iLump ) + iLumpSize );
	}

	static std::map<int, void( *)(int, CLumpManager*)>& Get()
	{
		// if there is a lump id without a fix func, give it the default func
		if ( m_bFirstInitialization ) // no need to run the loop again a second time
									  // TODO: find a way to this when static initialization occurs?
		{
			m_bFirstInitialization = false;

			for ( int i = 0; i < HEADER_LUMPS; i++ )
			{
				if ( m_FunctionMap.find( i ) == m_FunctionMap.end() ) // didn't find the id
					m_FunctionMap.emplace( i, &DefaultFixLumpFunc );
			}
		}			  		

		return m_FunctionMap;
	}

private:
	// The fix function calls must be called by their lump ids in an ascending order
	// some lumps (i.e. gamelumps) need this in order to find its current new offset in the bsp file
	static std::map<int, void( *)(int, CLumpManager*)> m_FunctionMap;
	static bool m_bFirstInitialization;
};

std::map<int, void( *)(int, CLumpManager*)> CFixLumpFunc::m_FunctionMap;
bool CFixLumpFunc::m_bFirstInitialization = true;

bool IsBspFile( uint8_t* pBuffer )
{
	if ( !pBuffer )
		return false;

	dheader_t* pBsp = (dheader_t*) pBuffer;

	return pBsp->ident == IDBSPHEADER && pBsp->version == BSPVERSION;
}

FIX_LUMP_FUNC( LUMP_FACES )
{
	cso2dface_t* pCso2Face = pLumpManager->GetLumpDataById<cso2dface_t>( FIX_LUMP_ID );
	int iCso2FacesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2FacesSize % sizeof( cso2dface_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewFaces = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iFaces = iCso2FacesSize / sizeof( cso2dface_t );
	vNewFaces.resize( iFaces * sizeof( dface_t ) );

	dface_t* pNewFacesLump = (dface_t*) vNewFaces.data();

	for ( uint32_t i = 0; i < iFaces; i++ )
	{
		if ( pCso2Face[i].planenum > USHRT_MAX )
			DBG_PRINTF( "detected possible planenum overflow. original: %X (%u)\n", pCso2Face[i].planenum, pCso2Face[i].planenum );
		if ( pCso2Face[i].texinfo > USHRT_MAX )
			DBG_PRINTF( "detected possible texinfo overflow. original: %X (%u)\n", pCso2Face[i].texinfo, pCso2Face[i].texinfo );
		if ( pCso2Face[i].dispinfo > USHRT_MAX )
			DBG_PRINTF( "detected possible dispinfo overflow. original: %X (%u)\n", pCso2Face[i].dispinfo, pCso2Face[i].dispinfo );
		if ( pCso2Face[i].surfaceFogVolumeID > USHRT_MAX )
			DBG_PRINTF( "detected possible surfaceFogVolumeID overflow. original: %X (%u)\n", pCso2Face[i].surfaceFogVolumeID, pCso2Face[i].surfaceFogVolumeID );
		if ( pCso2Face[i].m_NumPrims > USHRT_MAX )
			DBG_PRINTF( "detected possible m_NumPrims overflow. original: %X (%u)\n", pCso2Face[i].m_NumPrims, pCso2Face[i].m_NumPrims );
		if ( pCso2Face[i].firstPrimID > USHRT_MAX )
			DBG_PRINTF( "detected possible firstPrimID overflow. original: %X (%u)\n", pCso2Face[i].firstPrimID, pCso2Face[i].firstPrimID );

		pNewFacesLump[i].planenum = pCso2Face[i].planenum;
		pNewFacesLump[i].side = pCso2Face[i].side;
		pNewFacesLump[i].onNode = pCso2Face[i].onNode;
		pNewFacesLump[i].firstedge = pCso2Face[i].firstedge;
		pNewFacesLump[i].numedges = pCso2Face[i].numedges;
		pNewFacesLump[i].texinfo = pCso2Face[i].texinfo;
		pNewFacesLump[i].dispinfo = pCso2Face[i].dispinfo;
		pNewFacesLump[i].surfaceFogVolumeID = pCso2Face[i].surfaceFogVolumeID;
		memcpy( pNewFacesLump[i].styles, pCso2Face[i].styles, sizeof( pNewFacesLump[i].styles ) );
		pNewFacesLump[i].lightofs = pCso2Face[i].lightofs;
		pNewFacesLump[i].area = pCso2Face[i].area;
		pNewFacesLump[i].m_LightmapTextureMinsInLuxels[0] = pCso2Face[i].m_LightmapTextureMinsInLuxels[0];
		pNewFacesLump[i].m_LightmapTextureMinsInLuxels[1] = pCso2Face[i].m_LightmapTextureMinsInLuxels[1];
		pNewFacesLump[i].m_LightmapTextureSizeInLuxels[0] = pCso2Face[i].m_LightmapTextureSizeInLuxels[0];
		pNewFacesLump[i].m_LightmapTextureSizeInLuxels[1] = pCso2Face[i].m_LightmapTextureSizeInLuxels[1];
		pNewFacesLump[i].origFace = pCso2Face[i].origFace;
		pNewFacesLump[i].m_NumPrims = pCso2Face[i].m_NumPrims;
		pNewFacesLump[i].firstPrimID = pCso2Face[i].firstPrimID;
		pNewFacesLump[i].smoothingGroups = pCso2Face[i].smoothingGroups;
	}
}

FIX_LUMP_FUNC_EXISTING( LUMP_FACES_HDR, LUMP_FACES )
FIX_LUMP_FUNC_EXISTING( LUMP_ORIGINALFACES, LUMP_FACES )

FIX_LUMP_FUNC( LUMP_NODES )
{
	cso2dnode_t* pCso2Node = pLumpManager->GetLumpDataById<cso2dnode_t>( FIX_LUMP_ID );
	int iCso2NodesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2NodesSize % sizeof( cso2dnode_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewNodes = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iNodes = iCso2NodesSize / sizeof( cso2dnode_t );
	vNewNodes.resize( iNodes * sizeof( dnode_t ) );

	dnode_t* pNewNodes = (dnode_t*) vNewNodes.data();

	for ( uint32_t i = 0; i < iNodes; i++ )
	{
		for ( int j = 0; j < sizeof( pCso2Node[i].mins ) / sizeof( long ); j++ )
			if ( pCso2Node[i].mins[j] > USHRT_MAX )
				DBG_PRINTF( "detected possible mins[%i] overflow. original: %X (%u) index %u\n", j, pCso2Node[i].mins[j], pCso2Node[i].mins[j], i );
		for ( int j = 0; j < sizeof( pCso2Node[i].maxs ) / sizeof( long ); j++ )
			if ( pCso2Node[i].maxs[j] > USHRT_MAX )
				DBG_PRINTF( "detected possible maxs[%i] overflow. original: %X (%u) index %u\n", j, pCso2Node[i].maxs[j], pCso2Node[i].maxs[j], i );
		if ( pCso2Node[i].firstface > USHRT_MAX )
			DBG_PRINTF( "detected possible firstface overflow. original: %X (%u) index %u\n", pCso2Node[i].firstface, pCso2Node[i].firstface, i );
		if ( pCso2Node[i].numfaces > USHRT_MAX )
			DBG_PRINTF( "detected possible numfaces overflow. original: %X (%u) index %u\n", pCso2Node[i].numfaces, pCso2Node[i].numfaces, i );
		if ( pCso2Node[i].area > USHRT_MAX )
			DBG_PRINTF( "detected possible area overflow. original: %X (%u) index %u\n", pCso2Node[i].area, pCso2Node[i].area, i );

		pNewNodes[i].planenum = pCso2Node[i].planenum;

		for ( int j = 0; j < sizeof( pCso2Node[i].children ) / sizeof( int ); j++ )
			pNewNodes[i].children[j] = pCso2Node[i].children[j];

		for ( int j = 0; j < sizeof( pCso2Node[i].mins ) / sizeof( long ); j++ )
			pNewNodes[i].mins[j] = pCso2Node[i].mins[j];

		for ( int j = 0; j < sizeof( pCso2Node[i].maxs ) / sizeof( long ); j++ )
			pNewNodes[i].maxs[j] = pCso2Node[i].maxs[j];

		pNewNodes[i].firstface = pCso2Node[i].firstface;
		pNewNodes[i].numfaces = pCso2Node[i].numfaces;
		pNewNodes[i].area = pCso2Node[i].area;
	}
}

void FixLeafsLumpV0( int iLumpId, CLumpManager* pLumpManager )
{
	cso2dleaf_version_0_t* pCso2Leaves = pLumpManager->GetLumpDataById<cso2dleaf_version_0_t>( iLumpId );
	int iCso2LeafsSize = pLumpManager->GetLumpSizeById( iLumpId );

	if ( iCso2LeafsSize % sizeof( cso2dleaf_version_0_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewLeafs = pLumpManager->GetNewLumpDataById( iLumpId );

	uint32_t iLeaves = iCso2LeafsSize / sizeof( cso2dleaf_version_0_t );
	vNewLeafs.resize( iLeaves * sizeof( dleaf_version_0_t ) );

	dleaf_version_0_t* pNewLeaves = (dleaf_version_0_t*) vNewLeafs.data();

	for ( uint32_t i = 0; i < iLeaves; i++ )
	{
		if ( pCso2Leaves[i].cluster > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].cluster, pCso2Leaves[i].cluster, i );
		for ( int j = 0; j < sizeof( pCso2Leaves[i].mins ) / sizeof( long ); j++ )
			if ( pCso2Leaves[i].mins[j] > USHRT_MAX )
				DBG_PRINTF( "detected possible mins[%i] overflow. original: %X (%u) index %u\n", j, pCso2Leaves[i].mins[j], pCso2Leaves[i].mins[j], i );
		for ( int j = 0; j < sizeof( pCso2Leaves[i].mins ) / sizeof( long ); j++ )
			if ( pCso2Leaves[i].maxs[j] > USHRT_MAX )
				DBG_PRINTF( "detected possible maxs[%i] overflow. original: %X (%u) index %u\n", j, pCso2Leaves[i].maxs[j], pCso2Leaves[i].maxs[j], i );
		if ( pCso2Leaves[i].firstleafface > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].firstleafface, pCso2Leaves[i].firstleafface, i );
		if ( pCso2Leaves[i].numleaffaces > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].numleaffaces, pCso2Leaves[i].numleaffaces, i );
		if ( pCso2Leaves[i].firstleafbrush > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].firstleafbrush, pCso2Leaves[i].firstleafbrush, i );
		if ( pCso2Leaves[i].numleafbrushes > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].numleafbrushes, pCso2Leaves[i].numleafbrushes, i );

		pNewLeaves[i].contents = pCso2Leaves[i].contents;
		pNewLeaves[i].cluster = pCso2Leaves[i].cluster;
		pNewLeaves[i].bf = pCso2Leaves[i].bf;
		pNewLeaves[i].area = pCso2Leaves[i].area;
		pNewLeaves[i].flags = pCso2Leaves[i].flags;

		for ( int j = 0; j < sizeof( pCso2Leaves[i].mins ) / sizeof( long ); j++ )
			pNewLeaves[i].mins[j] = pCso2Leaves[i].mins[j];

		for ( int j = 0; j < sizeof( pCso2Leaves[i].maxs ) / sizeof( long ); j++ )
			pNewLeaves[i].maxs[j] = pCso2Leaves[i].maxs[j];

		pNewLeaves[i].firstleafface = pCso2Leaves[i].firstleafface;
		pNewLeaves[i].numleaffaces = pCso2Leaves[i].numleaffaces;
		pNewLeaves[i].firstleafbrush = pCso2Leaves[i].firstleafbrush;
		pNewLeaves[i].numleafbrushes = pCso2Leaves[i].numleafbrushes;
		pNewLeaves[i].leafWaterDataID = pCso2Leaves[i].leafWaterDataID;

		memcpy( &pNewLeaves[i].m_AmbientLighting, &pCso2Leaves[i].m_AmbientLighting, sizeof( pCso2Leaves[i].m_AmbientLighting ) );
	}
}

void FixLeafsLumpV1( int iLumpId, CLumpManager* pLumpManager )
{														
	cso2dleaf_t* pCso2Leaves = pLumpManager->GetLumpDataById<cso2dleaf_t>( iLumpId );
	int iCso2LeafsSize = pLumpManager->GetLumpSizeById( iLumpId );

	if ( iCso2LeafsSize % sizeof( cso2dleaf_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewLeafs = pLumpManager->GetNewLumpDataById( iLumpId );

	uint32_t iLeaves = iCso2LeafsSize / sizeof( cso2dleaf_t );
	vNewLeafs.resize( iLeaves * sizeof( dleaf_t ) );

	dleaf_t* pNewLeaves = (dleaf_t*) vNewLeafs.data();

	for ( uint32_t i = 0; i < iLeaves; i++ )
	{
		if ( pCso2Leaves[i].cluster > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].cluster, pCso2Leaves[i].cluster, i );
		for ( int j = 0; j < sizeof( pCso2Leaves[i].mins ) / sizeof( long ); j++ )
			if ( pCso2Leaves[i].mins[j] > USHRT_MAX )
				DBG_PRINTF( "detected possible mins[%i] overflow. original: %X (%u) index %u\n", j, pCso2Leaves[i].mins[j], pCso2Leaves[i].mins[j], i );
		for ( int j = 0; j < sizeof( pCso2Leaves[i].mins ) / sizeof( long ); j++ )
			if ( pCso2Leaves[i].maxs[j] > USHRT_MAX )
				DBG_PRINTF( "detected possible maxs[%i] overflow. original: %X (%u) index %u\n", j, pCso2Leaves[i].maxs[j], pCso2Leaves[i].maxs[j], i );
		if ( pCso2Leaves[i].firstleafface > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].firstleafface, pCso2Leaves[i].firstleafface, i );
		if ( pCso2Leaves[i].numleaffaces > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].numleaffaces, pCso2Leaves[i].numleaffaces, i );
		if ( pCso2Leaves[i].firstleafbrush > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].firstleafbrush, pCso2Leaves[i].firstleafbrush, i );
		if ( pCso2Leaves[i].numleafbrushes > USHRT_MAX )
			DBG_PRINTF( "detected possible cluster overflow. original: %X (%u) index %u\n", pCso2Leaves[i].numleafbrushes, pCso2Leaves[i].numleafbrushes, i );

		pNewLeaves[i].contents = pCso2Leaves[i].contents;
		pNewLeaves[i].cluster = pCso2Leaves[i].cluster;
		pNewLeaves[i].bf = pCso2Leaves[i].bf;
		pNewLeaves[i].area = pCso2Leaves[i].area;
		pNewLeaves[i].flags = pCso2Leaves[i].flags;

		for ( int j = 0; j < sizeof( pCso2Leaves[i].mins ) / sizeof( long ); j++ )
			pNewLeaves[i].mins[j] = pCso2Leaves[i].mins[j];

		for ( int j = 0; j < sizeof( pCso2Leaves[i].maxs ) / sizeof( long ); j++ )
			pNewLeaves[i].maxs[j] = pCso2Leaves[i].maxs[j];

		pNewLeaves[i].firstleafface = pCso2Leaves[i].firstleafface;
		pNewLeaves[i].numleaffaces = pCso2Leaves[i].numleaffaces;
		pNewLeaves[i].firstleafbrush = pCso2Leaves[i].firstleafbrush;
		pNewLeaves[i].numleafbrushes = pCso2Leaves[i].numleafbrushes;
		pNewLeaves[i].leafWaterDataID = pCso2Leaves[i].leafWaterDataID;
	}
}

FIX_LUMP_FUNC( LUMP_LEAFS )
{
	int iLeafVersion = pLumpManager->GetLumpVersionById( FIX_LUMP_ID );
	DBG_PRINTF( "Leafs version is %i\n", iLeafVersion );

	if ( iLeafVersion == 0 )
	{ 
		FixLeafsLumpV0( FIX_LUMP_ID, FIX_LUMP_MANAGER );
	}
	else if ( iLeafVersion == 1 )
	{ 
		FixLeafsLumpV1( FIX_LUMP_ID, FIX_LUMP_MANAGER );
	}
	else
	{
		DBG_PRINTF( "Invalid leaf version!\n" );
		assert( false );
	}
}

FIX_LUMP_FUNC( LUMP_EDGES )
{
	cso2dedge_t* pCso2Edges = pLumpManager->GetLumpDataById<cso2dedge_t>( FIX_LUMP_ID );
	int iCso2EdgesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2EdgesSize % sizeof( cso2dedge_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewEdges = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iEdges = iCso2EdgesSize / sizeof( cso2dedge_t );
	vNewEdges.resize( iEdges * sizeof( dedge_t ) );

	dedge_t* pNewEdges = (dedge_t*) vNewEdges.data();

	for ( uint32_t i = 0; i < iEdges; i++ )
	{
		for ( int j = 0; j < sizeof( pCso2Edges[i].v ) / sizeof( unsigned long ); j++ )
			if ( pCso2Edges[i].v[j] > USHRT_MAX )
				DBG_PRINTF( "detected possible edge[i] overflow. original: %X (%u) index %u\n", j, pCso2Edges[i].v[j], pCso2Edges[i].v[j], i );

		for ( int j = 0; j < sizeof( pCso2Edges[i].v ) / sizeof( unsigned long ); j++ )
			pNewEdges[i].v[j] = pCso2Edges[i].v[j];
	}
}

FIX_LUMP_FUNC( LUMP_LEAFFACES )
{		   
	unsigned long* pCso2LeafFaces = pLumpManager->GetLumpDataById<unsigned long>( FIX_LUMP_ID );
	int iCso2LeafFacesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2LeafFacesSize % sizeof( unsigned long ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewLeafFaces = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iLeafFaces = iCso2LeafFacesSize / sizeof( unsigned long );
	vNewLeafFaces.resize( iLeafFaces * sizeof( unsigned short ) );

	unsigned short* pNewLeafFaces = (unsigned short*) vNewLeafFaces.data();

	for ( uint32_t i = 0; i < iLeafFaces; i++ )
	{
		if ( pCso2LeafFaces[i] > USHRT_MAX )
			DBG_PRINTF( "detected possible leafface overflow. original: %X (%u) index %u\n", pCso2LeafFaces[i], pCso2LeafFaces[i], i );

		pNewLeafFaces[i] = pCso2LeafFaces[i];
	}
}

FIX_LUMP_FUNC( LUMP_LEAFBRUSHES )
{	   
	unsigned long* pCso2LeafBrushes = pLumpManager->GetLumpDataById<unsigned long>( FIX_LUMP_ID );
	int iCso2LeafBrushesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2LeafBrushesSize % sizeof( unsigned long ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewLeafBrushes = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iLeafBrushes = iCso2LeafBrushesSize / sizeof( unsigned long );
	vNewLeafBrushes.resize( iLeafBrushes * sizeof( unsigned short ) );

	unsigned short* pNewLeafBrushes = (unsigned short*) vNewLeafBrushes.data();

	for ( uint32_t i = 0; i < iLeafBrushes; i++ )
	{
		if ( pCso2LeafBrushes[i] > USHRT_MAX )
			DBG_PRINTF( "detected possible leafbrush overflow. original: %X (%u) index %u\n", pCso2LeafBrushes[i], pCso2LeafBrushes[i], i );

		pNewLeafBrushes[i] = pCso2LeafBrushes[i];
	}
}

FIX_LUMP_FUNC( LUMP_BRUSHSIDES )
{
	cso2dbrushside_t* pCso2BrushSide = pLumpManager->GetLumpDataById<cso2dbrushside_t>( FIX_LUMP_ID );
	int iCso2BrushSidesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2BrushSidesSize % sizeof( cso2dbrushside_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewBrushSides = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iBrushSides = iCso2BrushSidesSize / sizeof( cso2dbrushside_t );
	vNewBrushSides.resize( iBrushSides * sizeof( dbrushside_t ) );

	dbrushside_t* pNewBrushSides = (dbrushside_t*) vNewBrushSides.data();

	for ( uint32_t i = 0; i < iBrushSides; i++ )
	{
		if ( pCso2BrushSide[i].planenum > USHRT_MAX )
			DBG_PRINTF( "detected possible planenum overflow. original: %X (%u) index %u\n", pCso2BrushSide[i].planenum, pCso2BrushSide[i].planenum, i );
		if ( pCso2BrushSide[i].texinfo > USHRT_MAX )
			DBG_PRINTF( "detected possible texinfo overflow. original: %X (%u) index %u\n", pCso2BrushSide[i].texinfo, pCso2BrushSide[i].texinfo, i );

		pNewBrushSides[i].planenum = pCso2BrushSide[i].planenum;
		pNewBrushSides[i].texinfo = pCso2BrushSide[i].texinfo;
		pNewBrushSides[i].dispinfo = pCso2BrushSide[i].dispinfo;
		pNewBrushSides[i].bevel = pCso2BrushSide[i].bevel;
	}
}

FIX_LUMP_FUNC( LUMP_AREAPORTALS )
{
	cso2dareaportal_t* pCso2AreaPortal = pLumpManager->GetLumpDataById<cso2dareaportal_t>( FIX_LUMP_ID );
	int iCso2AreaPortalsSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2AreaPortalsSize % sizeof( cso2dareaportal_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewAreaPortals = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iAreaPortals = iCso2AreaPortalsSize / sizeof( cso2dareaportal_t );
	vNewAreaPortals.resize( iAreaPortals * sizeof( dareaportal_t ) );

	dareaportal_t* pNewAreaPortals = (dareaportal_t*) vNewAreaPortals.data();

	for ( uint32_t i = 0; i < iAreaPortals; i++ )
	{
		if ( pCso2AreaPortal[i].m_PortalKey > USHRT_MAX )
			DBG_PRINTF( "detected possible m_PortalKey overflow. original: %X (%u) index %u\n", pCso2AreaPortal[i].m_PortalKey, pCso2AreaPortal[i].m_PortalKey, i );
		if ( pCso2AreaPortal[i].otherarea > USHRT_MAX )
			DBG_PRINTF( "detected possible otherarea overflow. original: %X (%u) index %u\n", pCso2AreaPortal[i].otherarea, pCso2AreaPortal[i].otherarea, i );
		if ( pCso2AreaPortal[i].m_FirstClipPortalVert > USHRT_MAX )
			DBG_PRINTF( "detected possible m_FirstClipPortalVert overflow. original: %X (%u) index %u\n", pCso2AreaPortal[i].m_FirstClipPortalVert, pCso2AreaPortal[i].m_FirstClipPortalVert, i );
		if ( pCso2AreaPortal[i].m_nClipPortalVerts > USHRT_MAX )
			DBG_PRINTF( "detected possible m_nClipPortalVerts overflow. original: %X (%u) index %u\n", pCso2AreaPortal[i].m_nClipPortalVerts, pCso2AreaPortal[i].m_nClipPortalVerts, i );

		pNewAreaPortals[i].m_PortalKey = pCso2AreaPortal[i].m_PortalKey;
		pNewAreaPortals[i].otherarea = pCso2AreaPortal[i].otherarea;
		pNewAreaPortals[i].m_FirstClipPortalVert = pCso2AreaPortal[i].m_FirstClipPortalVert;
		pNewAreaPortals[i].m_nClipPortalVerts = pCso2AreaPortal[i].m_nClipPortalVerts;
		pNewAreaPortals[i].planenum = pCso2AreaPortal[i].planenum;
	}
}

void FixGameLumpLeafList( uint8_t* pInBuffer, uint8_t* pOutBuffer )
{
	int iLeavesCount = *(int*) pInBuffer;

	if ( iLeavesCount <= 0 )
	{
		DBG_PRINTF( "iLeavesCount is %i!\n", iLeavesCount );
		return;
	}

	*(int*) pOutBuffer = iLeavesCount;

	CSO2StaticPropLeafLump_t* pLeafList = (CSO2StaticPropLeafLump_t*) ((uintptr_t) pInBuffer + sizeof( int ));
	StaticPropLeafLump_t* pNewLeafList = (StaticPropLeafLump_t*) ((uintptr_t) pOutBuffer + sizeof( int ));

	for ( int i = 0; i < iLeavesCount; i++ )
	{
		if ( pLeafList[i].m_Leaf > USHRT_MAX )
			DBG_PRINTF( "detected possible m_Leaf overflow. original: %X (%u) index %u\n", pLeafList[i].m_Leaf, pLeafList[i].m_Leaf, i );

		pNewLeafList[i].m_Leaf = pLeafList[i].m_Leaf;
	}
}

void FixGameLumpModels( uint8_t* pInBuffer, uint8_t* pOutBuffer, int lumpVer )
{
	int iModelsCount = *(int*) pInBuffer;

	if ( iModelsCount <= 0 )
	{
		DBG_PRINTF( "iModelsCount is %i!\n", iModelsCount );
		return;
	}

	*(int*) pOutBuffer = iModelsCount;

	if ( lumpVer == 7 )
	{
		CSO2StaticPropLump_t* pModels = (CSO2StaticPropLump_t*) ((uintptr_t) pInBuffer + sizeof( int ));
		StaticPropLumpV6_t* pNewModels = (StaticPropLumpV6_t*) ((uintptr_t) pOutBuffer + sizeof( int ));

		for ( int i = 0; i < iModelsCount; i++ )
		{
			if ( pModels[i].m_FirstLeaf > USHRT_MAX )
				DBG_PRINTF( "detected possible m_FirstLeaf overflow. original: %X (%u) index %u\n", pModels[i].m_FirstLeaf, pModels[i].m_FirstLeaf, i );
			if ( pModels[i].m_LeafCount > USHRT_MAX )
				DBG_PRINTF( "detected possible m_LeafCount overflow. original: %X (%u) index %u\n", pModels[i].m_LeafCount, pModels[i].m_LeafCount, i );

			for ( int j = 0; j < sizeof( pModels[i].m_Origin ) / sizeof( float ); j++ )
				pNewModels[i].m_Origin[j] = pModels[i].m_Origin[j];

			for ( int j = 0; j < sizeof( pModels[i].m_Angles ) / sizeof( float ); j++ )
				pNewModels[i].m_Angles[j] = pModels[i].m_Angles[j];

			pNewModels[i].m_PropType = pModels[i].m_PropType;
			pNewModels[i].m_FirstLeaf = pModels[i].m_FirstLeaf;
			pNewModels[i].m_LeafCount = pModels[i].m_LeafCount;
			pNewModels[i].m_Solid = pModels[i].m_Solid;
			pNewModels[i].m_Skin = pModels[i].m_Skin;
			pNewModels[i].m_FadeMinDist = pModels[i].m_FadeMinDist;
			pNewModels[i].m_FadeMaxDist = pModels[i].m_FadeMaxDist;

			for ( int j = 0; j < sizeof( pModels[i].m_LightingOrigin ) / sizeof( float ); j++ )
				pNewModels[i].m_LightingOrigin[j] = pModels[i].m_LightingOrigin[j];

			pNewModels[i].m_flForcedFadeScale = pModels[i].m_flForcedFadeScale;
			pNewModels[i].m_nMinDXLevel = pModels[i].m_nMinDXLevel;
			pNewModels[i].m_nMaxDXLevel = pModels[i].m_nMaxDXLevel;

			pNewModels[i].m_Flags = pModels[i].m_Flags;
		}
	}
	else
	{
		CSO2StaticPropLumpV6_t* pModels = (CSO2StaticPropLumpV6_t*) ((uintptr_t) pInBuffer + sizeof( int ));
		StaticPropLumpV6_t* pNewModels = (StaticPropLumpV6_t*) ((uintptr_t) pOutBuffer + sizeof( int ));

		for ( int i = 0; i < iModelsCount; i++ )
		{
			if ( pModels[i].m_FirstLeaf > USHRT_MAX )
				DBG_PRINTF( "detected possible m_FirstLeaf overflow. original: %X (%u) index %u\n", pModels[i].m_FirstLeaf, pModels[i].m_FirstLeaf, i );
			if ( pModels[i].m_LeafCount > USHRT_MAX )
				DBG_PRINTF( "detected possible m_LeafCount overflow. original: %X (%u) index %u\n", pModels[i].m_LeafCount, pModels[i].m_LeafCount, i );

			for ( int j = 0; j < sizeof( pModels[i].m_Origin ) / sizeof( float ); j++ )
				pNewModels[i].m_Origin[j] = pModels[i].m_Origin[j];

			for ( int j = 0; j < sizeof( pModels[i].m_Angles ) / sizeof( float ); j++ )
				pNewModels[i].m_Angles[j] = pModels[i].m_Angles[j];

			pNewModels[i].m_PropType = pModels[i].m_PropType;
			pNewModels[i].m_FirstLeaf = pModels[i].m_FirstLeaf;
			pNewModels[i].m_LeafCount = pModels[i].m_LeafCount;
			pNewModels[i].m_Solid = pModels[i].m_Solid;
			pNewModels[i].m_Skin = pModels[i].m_Skin;
			pNewModels[i].m_FadeMinDist = pModels[i].m_FadeMinDist;
			pNewModels[i].m_FadeMaxDist = pModels[i].m_FadeMaxDist;

			for ( int j = 0; j < sizeof( pModels[i].m_LightingOrigin ) / sizeof( float ); j++ )
				pNewModels[i].m_LightingOrigin[j] = pModels[i].m_LightingOrigin[j];

			pNewModels[i].m_flForcedFadeScale = pModels[i].m_flForcedFadeScale;
			pNewModels[i].m_nMinDXLevel = pModels[i].m_nMinDXLevel;
			pNewModels[i].m_nMaxDXLevel = pModels[i].m_nMaxDXLevel;
			pNewModels[i].m_Flags = pModels[i].m_Flags;
		}
	}
}

void FixGameLumpDynamicModels( uint8_t* pInBuffer, uint8_t* pOutBuffer )
{
	int iDynModelsCount = *(int*) pInBuffer;

	if ( iDynModelsCount <= 0 )
	{
		DBG_PRINTF( "iDynModelsCount is %i!\n", iDynModelsCount );
		return;
	}

	*(int*) pOutBuffer = iDynModelsCount;

	CSO2DetailObjectLump_t* pModels = (CSO2DetailObjectLump_t*) ((uintptr_t) pInBuffer + sizeof( int ));
	DetailObjectLump_t* pNewModels = (DetailObjectLump_t*) ((uintptr_t) pOutBuffer + sizeof( int ));

	for ( int i = 0; i < iDynModelsCount; i++ )
	{
		if ( pModels[i].m_Leaf > USHRT_MAX )
			DBG_PRINTF( "detected possible m_Leaf overflow. original: %X (%u) index %u\n", pModels[i].m_Leaf, pModels[i].m_Leaf, i );

		for ( int j = 0; j < sizeof( pModels[i].m_Origin ) / sizeof( float ); j++ )
			pNewModels[i].m_Origin[j] = pModels[i].m_Origin[j];

		for ( int j = 0; j < sizeof( pModels[i].m_Angles ) / sizeof( float ); j++ )
			pNewModels[i].m_Angles[j] = pModels[i].m_Angles[j];

		pNewModels[i].m_DetailModel = pModels[i].m_DetailModel;
		pNewModels[i].m_Leaf = pModels[i].m_Leaf;
		pNewModels[i].m_Lighting = pModels[i].m_Lighting;
		pNewModels[i].m_LightStyles = pModels[i].m_LightStyles;
		pNewModels[i].m_LightStyleCount = pModels[i].m_LightStyleCount;
		pNewModels[i].m_SwayAmount = pModels[i].m_SwayAmount;
		pNewModels[i].m_ShapeAngle = pModels[i].m_ShapeAngle;
		pNewModels[i].m_ShapeSize = pModels[i].m_ShapeSize;
		pNewModels[i].m_Orientation = pModels[i].m_Orientation;
		pNewModels[i].m_Type = pModels[i].m_Type;
		pNewModels[i].m_flScale = pModels[i].m_flScale;
	}
}

#define MAKE_GAMELUMP(header) (dgamelump_t*)((uintptr_t) header + 4)

FIX_LUMP_FUNC( LUMP_GAME_LUMP )
{
	uint8_t* pGameLumpBuffer = pLumpManager->GetLumpDataById<uint8_t>( FIX_LUMP_ID );
	uint32_t iGameLumpsSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );		   
	std::vector<uint8_t>& vNewGameLump = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	dgamelumpheader_t* pHeader = (dgamelumpheader_t*) pGameLumpBuffer;
	dgamelump_t* pLump = MAKE_GAMELUMP( pHeader );

	dgamelump_t* pStaticPropsLump = nullptr;

	// we need access to the static props game lump so we can know long it is through its version
	for ( int i = 0; i < pHeader->lumpCount; i++ )
	{
		if ( pLump[i].id == GAMELUMP_STATIC_PROPS )
			pStaticPropsLump = &pLump[i];
	}

	// there are two different static prop lumps so we use the version to differentiate them
	int iStaticPropsLumpVersion = pStaticPropsLump->version;

	size_t staticLumpsSize = iStaticPropsLumpVersion == 7 ? sizeof( CSO2StaticPropLump_t ) : sizeof( CSO2StaticPropLumpV6_t );

	// calculate current and new offsets to use later
	uint32_t iModelDictOffset = *(int*) pGameLumpBuffer * sizeof( dgamelump_t ) + sizeof( dgamelumpheader_t );
	int iModelDictCount = *(int*) (pGameLumpBuffer + iModelDictOffset);

	uint32_t iLeafListOffset = iModelDictOffset + iModelDictCount * sizeof( StaticPropDictLump_t ) + sizeof( int );
	int iLeafListCount = *(int*) (pGameLumpBuffer + iLeafListOffset);

	uint32_t iModelsOffset = iLeafListOffset + iLeafListCount * sizeof( CSO2StaticPropLeafLump_t ) + sizeof( int );
	uint32_t iNewModelsOffset = iLeafListOffset + iLeafListCount * sizeof( StaticPropLeafLump_t ) + sizeof( int );
	int iModelsCount = *(int*) (pGameLumpBuffer + iModelsOffset);

	uint32_t iDynModelDictOffset = iModelsOffset + iModelsCount * staticLumpsSize + sizeof( int );
	uint32_t iNewDynModelDictOffset = iNewModelsOffset + iModelsCount * sizeof( StaticPropLumpV6_t ) + sizeof( int );
	int iDynModelDictCount = *(int*) (pGameLumpBuffer + iDynModelDictOffset);

	uint32_t iDynDetailSpritesOffset = iDynModelDictOffset + iDynModelDictCount * sizeof( DetailObjectDictLump_t ) + sizeof( int );
	uint32_t iNewDynDetailSpritesOffset = iNewDynModelDictOffset + iDynModelDictCount * sizeof( DetailObjectDictLump_t ) + sizeof( int );
	int iDynDetailSpritesCount = *(int*) (pGameLumpBuffer + iDynDetailSpritesOffset);

	uint32_t iDynModelsOffset = iDynDetailSpritesOffset + iDynDetailSpritesCount * sizeof( DetailSpriteDictLump_t ) + sizeof( int );
	uint32_t iNewDynModelsOffset = iNewDynDetailSpritesOffset + iDynDetailSpritesCount * sizeof( DetailSpriteDictLump_t ) + sizeof( int );
	int iDynModelsCount = *(int*) (pGameLumpBuffer + iDynModelsOffset);

	uint32_t iCalculatedBufferSize = iDynModelsOffset + iDynModelsCount * sizeof( CSO2DetailObjectLump_t ) + sizeof( int );
	uint32_t iNewBufferSize = iNewDynModelsOffset + iDynModelsCount * sizeof( DetailObjectLump_t ) + sizeof( int );

	// give our new lump the required space
	vNewGameLump.resize( iNewBufferSize );

	// copy the original game lump headers
	memcpy( vNewGameLump.data(), pGameLumpBuffer, pHeader->lumpCount * sizeof( dgamelump_t ) + sizeof( dgamelumpheader_t ) );

	dgamelumpheader_t* pNewHeader = (dgamelumpheader_t*) vNewGameLump.data();
	dgamelump_t* pNewLump = MAKE_GAMELUMP( pNewHeader );	   
	
	// we need these to calculate the lump offsets
	int iOrigLumpBase = pLumpManager->GetLumpOffsetById( FIX_LUMP_ID );
	int iNewLumpBase = pLumpManager->GetNewLumpOffsetById( FIX_LUMP_ID );

	// set the game lumps offsets to our own
	for ( int i = 0; i < pNewHeader->lumpCount; i++ )
	{
		if ( pNewLump[i].id == GAMELUMP_STATIC_PROPS )
			pNewLump[i].fileofs = iModelDictOffset + iNewLumpBase;
		else if ( pNewLump[i].id == GAMELUMP_DETAIL_PROPS )
			pNewLump[i].fileofs = iNewDynModelDictOffset + iNewLumpBase;
		else
			pNewLump[i].fileofs = pNewLump[i].fileofs - iOrigLumpBase + iNewLumpBase;
	}

	dgamelump_t* pNewStaticPropsLump = nullptr;

	for ( int i = 0; i < pNewHeader->lumpCount; i++ )
	{
		if ( pNewLump[i].id == GAMELUMP_STATIC_PROPS )
			pNewStaticPropsLump = &pNewLump[i];
	}

	// insert the StaticPropDictLump_t's
	memcpy( vNewGameLump.data() + iModelDictOffset, pGameLumpBuffer + iModelDictOffset, iModelDictCount * sizeof( StaticPropDictLump_t ) + sizeof( int ) );

	// fix and insert the StaticPropLeafLump_t's 
	FixGameLumpLeafList( pGameLumpBuffer + iLeafListOffset, vNewGameLump.data() + iLeafListOffset );

	// fix and insert the StaticPropLump_t's 
	FixGameLumpModels( pGameLumpBuffer + iModelsOffset, vNewGameLump.data() + iNewModelsOffset, iStaticPropsLumpVersion );

	// set the static props lump to our version
	pNewStaticPropsLump->version = 6;

	// insert the DetailObjectDictLump_t's
	memcpy( vNewGameLump.data() + iNewDynModelDictOffset, pGameLumpBuffer + iDynModelDictOffset, iDynModelDictCount * sizeof( DetailObjectDictLump_t ) + sizeof( int ) );
	
	// insert the DetailSpriteDictLump_t's
	memcpy( vNewGameLump.data() + iNewDynDetailSpritesOffset, pGameLumpBuffer + iDynDetailSpritesOffset, iDynDetailSpritesCount * sizeof( DetailSpriteDictLump_t ) + sizeof( int ) );
	
	// fix and insert the DetailObjectLump_t's 
	FixGameLumpDynamicModels( pGameLumpBuffer + iDynModelsOffset, vNewGameLump.data() + iNewDynModelsOffset );
}

FIX_LUMP_FUNC( LUMP_LEAFWATERDATA )
{	 	
	cso2dleafwaterdata_t* pCso2LeafWaterData = pLumpManager->GetLumpDataById<cso2dleafwaterdata_t>( FIX_LUMP_ID );
	int iCso2LeafWaterDataSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2LeafWaterDataSize % sizeof( cso2dleafwaterdata_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewLeafWaterData = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iLeavesWaterData = iCso2LeafWaterDataSize / sizeof( cso2dleafwaterdata_t );
	vNewLeafWaterData.resize( iLeavesWaterData * sizeof( dleafwaterdata_t ) );

	dleafwaterdata_t* pNewLeafWaterData = (dleafwaterdata_t*) vNewLeafWaterData.data();

	for ( uint32_t i = 0; i < iLeavesWaterData; i++ )
	{
		if ( pCso2LeafWaterData[i].surfaceTexInfoID > USHRT_MAX )
			DBG_PRINTF( "detected possible surfaceTexInfoID overflow. original: %X (%u) index %u\n", pCso2LeafWaterData[i].surfaceTexInfoID, pCso2LeafWaterData[i].surfaceTexInfoID, i );

		pNewLeafWaterData[i].surfaceZ = pCso2LeafWaterData[i].surfaceZ;
		pNewLeafWaterData[i].minZ = pCso2LeafWaterData[i].minZ;
		pNewLeafWaterData[i].surfaceTexInfoID = pCso2LeafWaterData[i].surfaceTexInfoID;
	}
}

FIX_LUMP_FUNC( LUMP_PRIMITIVES )
{
	cso2dprimitive_t* pCso2Primitive = pLumpManager->GetLumpDataById<cso2dprimitive_t>( FIX_LUMP_ID );
	int iCso2PrimitivesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2PrimitivesSize % sizeof( cso2dprimitive_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewPrimitives = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iPrimitives = iCso2PrimitivesSize / sizeof( cso2dprimitive_t );
	vNewPrimitives.resize( iPrimitives * sizeof( dprimitive_t ) );

	dprimitive_t* pNewPrimitives = (dprimitive_t*) vNewPrimitives.data();

	for ( uint32_t i = 0; i < iPrimitives; i++ )
	{
		if ( pCso2Primitive[i].type > USHRT_MAX )
			DBG_PRINTF( "detected possible type overflow. original: %X (%u) index %u\n", pCso2Primitive[i].type, pCso2Primitive[i].type, i );
		if ( pCso2Primitive[i].firstIndex > USHRT_MAX )
			DBG_PRINTF( "detected possible firstIndex overflow. original: %X (%u) index %u\n", pCso2Primitive[i].firstIndex, pCso2Primitive[i].firstIndex, i );
		if ( pCso2Primitive[i].indexCount > USHRT_MAX )
			DBG_PRINTF( "detected possible indexCount overflow. original: %X (%u) index %u\n", pCso2Primitive[i].indexCount, pCso2Primitive[i].indexCount, i );
		if ( pCso2Primitive[i].firstVert > USHRT_MAX )
			DBG_PRINTF( "detected possible firstVert overflow. original: %X (%u) index %u\n", pCso2Primitive[i].firstVert, pCso2Primitive[i].firstVert, i );
		if ( pCso2Primitive[i].vertCount > USHRT_MAX )
			DBG_PRINTF( "detected possible vertCount overflow. original: %X (%u) index %u\n", pCso2Primitive[i].vertCount, pCso2Primitive[i].vertCount, i );

		pNewPrimitives[i].type = pCso2Primitive[i].type;
		pNewPrimitives[i].firstIndex = pCso2Primitive[i].firstIndex;
		pNewPrimitives[i].indexCount = pCso2Primitive[i].indexCount;
		pNewPrimitives[i].firstVert = pCso2Primitive[i].firstVert;
		pNewPrimitives[i].vertCount = pCso2Primitive[i].vertCount;
	}
}

FIX_LUMP_FUNC( LUMP_PRIMINDICES )
{		
	unsigned long* pCso2PrimIndices = pLumpManager->GetLumpDataById<unsigned long>( FIX_LUMP_ID );
	int iCso2PrimIndicesSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2PrimIndicesSize % sizeof( unsigned long ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewPrimIndices = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iPrimIndices = iCso2PrimIndicesSize / sizeof( unsigned long );
	vNewPrimIndices.resize( iPrimIndices * sizeof( unsigned short ) );

	unsigned short* pNewPrimIndices = (unsigned short*) vNewPrimIndices.data();

	for ( uint32_t i = 0; i < iPrimIndices; i++ )
	{
		if ( pCso2PrimIndices[i] > USHRT_MAX )
			DBG_PRINTF( "detected possible prim indice overflow. original: %X (%u) index %u\n", pCso2PrimIndices[i], pCso2PrimIndices[i], i );

		pNewPrimIndices[i] = pCso2PrimIndices[i];
	}
}

FIX_LUMP_FUNC( LUMP_PAKFILE )
{
	uint8_t* pZipBuffer = pLumpManager->GetLumpDataById<uint8_t>( FIX_LUMP_ID );
	int iZipBufferSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	uintptr_t iOffset = iZipBufferSize - sizeof( ZIP_EndOfCentralDirRecord );
	ZIP_EndOfCentralDirRecord* pRecord = nullptr;
	bool bFoundRecord = false;

	// the record holds the number of files inside the zip
	for ( ; iOffset >= 0; iOffset-- )
	{
		pRecord = (ZIP_EndOfCentralDirRecord*) ((uintptr_t) pZipBuffer + iOffset);

		if ( pRecord->signature == CSO2_PKID( 5, 6 ) )
		{
			bFoundRecord = true;
			break;
		}
	}

	if ( !bFoundRecord )
		return;

	int numFilesInZip = pRecord->nCentralDirectoryEntries_Total;	 	

	std::vector<uint8_t>& vNewZipBuffer = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );	// the new zip buffer
	std::vector<uint8_t> vFileHeaderBuffer; // we can only place the file header (PK12) after local file header (PK34)
											// TODO: find a better way to do this?

	uintptr_t iCurrentOffset = 0;

	for ( int i = 0; i < numFilesInZip; i++ )
	{
		uint32_t iSignature = *(uint32_t*) ((uintptr_t) pZipBuffer + iCurrentOffset);

		// Making sure these are file entries, it should never fail
		if ( iSignature == CSO2_PKID( 3, 4 ) ) 
		{
			CSO2ZIP_LocalFileHeader* pLocalFileHeader = (CSO2ZIP_LocalFileHeader*) ((uintptr_t) pZipBuffer + iCurrentOffset);

			uint8_t* pData = (uint8_t*) ((uintptr_t) pLocalFileHeader
				+ sizeof( CSO2ZIP_LocalFileHeader )
				+ pLocalFileHeader->fileNameLength
				+ pLocalFileHeader->extraFieldLength);

			unsigned int iDataSize = pLocalFileHeader->uncompressedSize;

			CLZMA lzma;	 						   			

			// if either the zip file entry or vtf are compressed it will create a new buffer
			// with the same size of the file entry and copy the file entry to itself
			// setting this to true will make sure the function deletes the new buffer
			// TODO: think of a better way to do this?
			bool bIsCompressed = lzma.IsCompressed( pData );

			if ( bIsCompressed )
			{
				unsigned int iActualSize = lzma.GetActualSize( pData );
				uint8_t* pTemp = new uint8_t[iActualSize];

				lzma.Uncompress( pData, pTemp );

				iDataSize = iActualSize;
				pData = pTemp;
			}

			// it needs a separate check since the compressed vtf size isnt stored in the localfileheader (or the fileheader)
			bool bIsVtfCompressed = IsCompressedVtf( pData );

			if ( bIsVtfCompressed )
			{
				uint8_t* pTemp = new uint8_t[iDataSize];
				memcpy( pTemp, pData, iDataSize );

				if ( !DecompressVtf( pTemp, iDataSize ) )
				{
					DBG_PRINTF( "VTF decompression failed!\n" );
					assert( 0 );
				}

				pData = pTemp;
			}

			// most of the code is from source sdk's 'public/ziputils.cpp'
			// this header stores the data right next to it
			ZIP_LocalFileHeader hdr = { 0 };
			hdr.signature = PKID( 3, 4 );
			hdr.versionNeededToExtract = 10;  // This is the version that the winzip that I have writes.
			hdr.flags = 0;
			hdr.compressionMethod = 0; // NO COMPRESSION!
			hdr.lastModifiedTime = 0;
			hdr.lastModifiedDate = 0;

			CryptoPP::CRC32 crc32;
			crc32.CalculateDigest( (uint8_t*) &hdr.crc32, pData, iDataSize );

			char* pFilename = new char[pLocalFileHeader->fileNameLength];
			memcpy( pFilename, (void*) ((uintptr_t) pLocalFileHeader + sizeof( CSO2ZIP_LocalFileHeader )),
				pLocalFileHeader->fileNameLength );

			hdr.compressedSize = iDataSize;
			hdr.uncompressedSize = iDataSize;
			hdr.fileNameLength = pLocalFileHeader->fileNameLength;
			hdr.extraFieldLength = 0;

			uintptr_t iHeaderOffset = vNewZipBuffer.size();

			vNewZipBuffer.insert( vNewZipBuffer.end(), (uint8_t*) &hdr,
				(uint8_t*) ((uintptr_t) &hdr + sizeof( ZIP_LocalFileHeader )) );
			vNewZipBuffer.insert( vNewZipBuffer.end(), (uint8_t*) pFilename,
				(uint8_t*) ((uintptr_t) pFilename + pLocalFileHeader->fileNameLength) );
			vNewZipBuffer.insert( vNewZipBuffer.end(), pData,
				(uint8_t*) ((uintptr_t) pData + iDataSize) );

			// create a file header that will be placed after the local file headers		 
			ZIP_FileHeader fhdr = { 0 };
			fhdr.signature = PKID( 1, 2 );
			fhdr.versionMadeBy = 20;				// This is the version that the winzip that I have writes.
			fhdr.versionNeededToExtract = 10;	// This is the version that the winzip that I have writes.
			fhdr.flags = 0;
			fhdr.compressionMethod = 0;
			fhdr.lastModifiedTime = 0;
			fhdr.lastModifiedDate = 0;
			fhdr.crc32 = hdr.crc32;

			fhdr.compressedSize = iDataSize;
			fhdr.uncompressedSize = iDataSize;
			fhdr.fileNameLength = hdr.fileNameLength;
			fhdr.extraFieldLength = 0;
			fhdr.fileCommentLength = 0;
			fhdr.diskNumberStart = 0;
			fhdr.internalFileAttribs = 0;
			fhdr.externalFileAttribs = 0; // This is usually something, but zero is OK as if the input came from stdin
			fhdr.relativeOffsetOfLocalHeader = iHeaderOffset;

			vFileHeaderBuffer.insert( vFileHeaderBuffer.end(), (uint8_t*) &fhdr,
				(uint8_t*) ((uintptr_t) &fhdr + sizeof( ZIP_FileHeader )) );
			vFileHeaderBuffer.insert( vFileHeaderBuffer.end(), (uint8_t*) pFilename,
				(uint8_t*) ((uintptr_t) pFilename + fhdr.fileNameLength) );

			if ( bIsCompressed || bIsVtfCompressed )
				delete pData;

			iCurrentOffset += sizeof( CSO2ZIP_LocalFileHeader )
				+ pLocalFileHeader->fileNameLength
				+ pLocalFileHeader->extraFieldLength
				+ (bIsCompressed ? pLocalFileHeader->compressedSize : pLocalFileHeader->uncompressedSize);
		}
		else
		{
			DBG_PRINTF( "Invalid signature found at file index %i!\n", i );
			assert( 0 );
		}
	}

	unsigned int centralDirStart = vNewZipBuffer.size();

	// now we insert our file headers
	vNewZipBuffer.insert( vNewZipBuffer.end(), vFileHeaderBuffer.data(), vFileHeaderBuffer.data() + vFileHeaderBuffer.size() );

	unsigned int centralDirEnd = vNewZipBuffer.size();

	// our new dir record 
	ZIP_EndOfCentralDirRecord newDirRecord;
	newDirRecord.signature = PKID( 5, 6 );
	newDirRecord.numberOfThisDisk = 0;
	newDirRecord.numberOfTheDiskWithStartOfCentralDirectory = 0;
	newDirRecord.nCentralDirectoryEntries_ThisDisk = numFilesInZip;
	newDirRecord.nCentralDirectoryEntries_Total = numFilesInZip;
	newDirRecord.centralDirectorySize = centralDirEnd - centralDirStart;
	newDirRecord.startOfCentralDirOffset = centralDirStart;

	vNewZipBuffer.insert( vNewZipBuffer.end(), (uint8_t*) &newDirRecord,
		(uint8_t*) ((uintptr_t) &newDirRecord + sizeof( ZIP_EndOfCentralDirRecord )) );
}

FIX_LUMP_FUNC( LUMP_CUBEMAPS )
{	 	
	cso2dcubemapsample_t* pCso2Cubemap = pLumpManager->GetLumpDataById<cso2dcubemapsample_t>( FIX_LUMP_ID );
	int iCso2CubemapsSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	// aim_dust cubemap lump is smaller than everyone else by 4 bytes
	// maybe it should be implemented, although the map doesn't even run on cso2 (says that the lump size is funny)
	if ( iCso2CubemapsSize % sizeof( cso2dcubemapsample_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewCubemaps = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iCubemaps = iCso2CubemapsSize / sizeof( cso2dcubemapsample_t );
	vNewCubemaps.resize( iCubemaps * sizeof( dcubemapsample_t ) );

	dcubemapsample_t* pNewCubemaps = (dcubemapsample_t*) vNewCubemaps.data();

	for ( uint32_t i = 0; i < iCubemaps; i++ )
	{
		for ( int j = 0; j < sizeof( pCso2Cubemap[i].origin ) / sizeof( int ); j++ )
			pNewCubemaps[i].origin[j] = pCso2Cubemap[i].origin[j];

		pNewCubemaps[i].size = pCso2Cubemap[i].size;
	}
}

FIX_LUMP_FUNC( LUMP_OVERLAYS )
{
	cso2doverlay_t* pCso2Overlay = pLumpManager->GetLumpDataById<cso2doverlay_t>( FIX_LUMP_ID );
	int iCso2OverlaysSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2OverlaysSize % sizeof( cso2doverlay_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewOverlays = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iOverlays = iCso2OverlaysSize / sizeof( cso2doverlay_t );
	vNewOverlays.resize( iOverlays * sizeof( doverlay_t ) );

	doverlay_t* pNewOverlay = (doverlay_t*) vNewOverlays.data();

	for ( uint32_t i = 0; i < iOverlays; i++ )
	{
		if ( pCso2Overlay[i].nTexInfo > SHRT_MAX )
			DBG_PRINTF( "detected possible nTexInfo overflow. original: %X (%u) index %u\n", pCso2Overlay[i].nTexInfo, pCso2Overlay[i].nTexInfo, i );
		if ( pCso2Overlay[i].m_nFaceCountAndRenderOrder > USHRT_MAX )
			DBG_PRINTF( "detected possible m_nFaceCountAndRenderOrder overflow. original: %X (%u) index %u\n", pCso2Overlay[i].m_nFaceCountAndRenderOrder, pCso2Overlay[i].m_nFaceCountAndRenderOrder, i );

		pNewOverlay[i].nId = pCso2Overlay[i].nId;
		pNewOverlay[i].nTexInfo = pCso2Overlay[i].nTexInfo;
		pNewOverlay[i].m_nFaceCountAndRenderOrder = pCso2Overlay[i].m_nFaceCountAndRenderOrder;

		for ( int j = 0; j < sizeof( pCso2Overlay[i].aFaces ) / sizeof( int ); j++ )
			pNewOverlay[i].aFaces[j] = pCso2Overlay[i].aFaces[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].flU ) / sizeof( float ); j++ )
			pNewOverlay[i].flU[j] = pCso2Overlay[i].flU[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].flV ) / sizeof( float ); j++ )
			pNewOverlay[i].flV[j] = pCso2Overlay[i].flV[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].vecUVPoints ) / sizeof( Vector ); j++ )
			for ( int k = 0; k < sizeof( pCso2Overlay[i].vecUVPoints[j] ) / sizeof( float ); k++ )
				pNewOverlay[i].vecUVPoints[j][k] = pCso2Overlay[i].vecUVPoints[j][k];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].vecOrigin ) / sizeof( float ); j++ )
			pNewOverlay[i].vecOrigin[j] = pCso2Overlay[i].vecOrigin[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].vecBasisNormal ) / sizeof( float ); j++ )
			pNewOverlay[i].vecBasisNormal[j] = pCso2Overlay[i].vecBasisNormal[j];
	}
}

FIX_LUMP_FUNC( LUMP_WATEROVERLAYS )
{	 	
	cso2dwateroverlay_t* pCso2Overlay = pLumpManager->GetLumpDataById<cso2dwateroverlay_t>( FIX_LUMP_ID );
	int iCso2WaterOverlaysSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2WaterOverlaysSize % sizeof( cso2dwateroverlay_t ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewWaterOverlays = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iWaterOverlays = iCso2WaterOverlaysSize / sizeof( cso2dwateroverlay_t );
	vNewWaterOverlays.resize( iWaterOverlays * sizeof( dwateroverlay_t ) );

	dwateroverlay_t* pNewOverlay = (dwateroverlay_t*) vNewWaterOverlays.data();

	for ( uint32_t i = 0; i < iWaterOverlays; i++ )
	{
		if ( pCso2Overlay[i].nTexInfo > SHRT_MAX )
			DBG_PRINTF( "detected possible nTexInfo overflow. original: %X (%u) index %u\n", pCso2Overlay[i].nTexInfo, pCso2Overlay[i].nTexInfo, i );
		if ( pCso2Overlay[i].m_nFaceCountAndRenderOrder > USHRT_MAX )
			DBG_PRINTF( "detected possible m_nFaceCountAndRenderOrder overflow. original: %X (%u) index %u\n", pCso2Overlay[i].m_nFaceCountAndRenderOrder, pCso2Overlay[i].m_nFaceCountAndRenderOrder, i );

		pNewOverlay[i].nId = pCso2Overlay[i].nId;
		pNewOverlay[i].nTexInfo = pCso2Overlay[i].nTexInfo;
		pNewOverlay[i].m_nFaceCountAndRenderOrder = pCso2Overlay[i].m_nFaceCountAndRenderOrder;

		for ( int j = 0; j < sizeof( pCso2Overlay[i].aFaces ) / sizeof( int ); j++ )
			pNewOverlay[i].aFaces[j] = pCso2Overlay[i].aFaces[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].flU ) / sizeof( float ); j++ )
			pNewOverlay[i].flU[j] = pCso2Overlay[i].flU[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].flV ) / sizeof( float ); j++ )
			pNewOverlay[i].flV[j] = pCso2Overlay[i].flV[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].vecUVPoints ) / sizeof( Vector ); j++ )
			for ( int k = 0; k < sizeof( pCso2Overlay[i].vecUVPoints[j] ) / sizeof( float ); k++ )
				pNewOverlay[i].vecUVPoints[j][k] = pCso2Overlay[i].vecUVPoints[j][k];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].vecOrigin ) / sizeof( float ); j++ )
			pNewOverlay[i].vecOrigin[j] = pCso2Overlay[i].vecOrigin[j];
		for ( int j = 0; j < sizeof( pCso2Overlay[i].vecBasisNormal ) / sizeof( float ); j++ )
			pNewOverlay[i].vecBasisNormal[j] = pCso2Overlay[i].vecBasisNormal[j];
	}
}

FIX_LUMP_FUNC( LUMP_LEAFMINDISTTOWATER )
{
	unsigned long* pCso2LeafMinDist = pLumpManager->GetLumpDataById<unsigned long>( FIX_LUMP_ID );
	int iCso2LeafMinDistSize = pLumpManager->GetLumpSizeById( FIX_LUMP_ID );

	if ( iCso2LeafMinDistSize % sizeof( unsigned long ) )
		DBG_PRINTF( "Buffer has a bad size! Still going for it...\n" );

	std::vector<uint8_t>& vNewLeafMinDist = pLumpManager->GetNewLumpDataById( FIX_LUMP_ID );

	uint32_t iLeafMinDists = iCso2LeafMinDistSize / sizeof( unsigned long );
	vNewLeafMinDist.resize( iLeafMinDists * sizeof( unsigned short ) );

	unsigned short* pNewLeafMinDist = (unsigned short*) vNewLeafMinDist.data();

	for ( uint32_t i = 0; i < iLeafMinDists; i++ )
	{
		if ( pCso2LeafMinDist[i] > USHRT_MAX )
			DBG_PRINTF( "detected possible LeafMinDistToWater overflow. original: %X (%u) index %u\n", pCso2LeafMinDist[i], pCso2LeafMinDist[i], i );

		pNewLeafMinDist[i] = pCso2LeafMinDist[i];
	}
}

bool DecompressBsp( uint8_t*& pBuffer, uint32_t& iBufferSize, bool bShouldFixLumps )
{
	if ( !iBufferSize )
	{
		DBG_PRINTF( "iBufferSize is null, pretending it's fine\n" );
		return true;
	}

	CLumpManager lumpManager( pBuffer );
	
	if ( bShouldFixLumps )
	{
		for ( const auto& func : CFixLumpFunc::Get() )
			func.second( func.first, &lumpManager );
	}

	lumpManager.BuildNewLumpsHeaders();

	return lumpManager.CreateNewBspBuffer( pBuffer, iBufferSize );
}
