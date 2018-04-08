#pragma once

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines and structures for the BSP file format.
//
// $NoKeywords: $
//=============================================================================//

#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'V')		
#define BSPVERSION 100
#define CSO2_BSPVERSION 100

#define	HEADER_LUMPS		64

typedef unsigned char byte;

class Vector
{
public:
	// Members
	float x, y, z;

	Vector( float xx, float yy, float zz )
	{
		x = xx;
		y = yy;
		z = zz;
	}


	Vector( float xyz )
	{
		x = y = z = xyz;
	}

	inline float operator[]( int i ) const
	{
		return ((float*) this)[i];
	}

	inline float& operator[]( int i )
	{
		return ((float*) this)[i];
	}
};

class Vector2D
{
public:
	// Members
	float x, y;

	inline float operator[]( int i ) const
	{
		return ((float*) this)[i];
	}

	inline float& operator[]( int i )
	{
		return ((float*) this)[i];
	}
};

class QAngle
{
public:
	// Members
	float x, y, z;

	inline float operator[]( int i ) const
	{
		return ((float*) this)[i];
	}

	inline float& operator[]( int i )
	{
		return ((float*) this)[i];
	}
};					 

struct ColorRGBExp32
{
	byte r, g, b;
	signed char exponent;
};

enum
{
	LUMP_ENTITIES					= 0,	// *
	LUMP_PLANES						= 1,	// *
	LUMP_TEXDATA					= 2,	// *
	LUMP_VERTEXES					= 3,	// *
	LUMP_VISIBILITY					= 4,	// *
	LUMP_NODES						= 5,	// *
	LUMP_TEXINFO					= 6,	// *
	LUMP_FACES						= 7,	// *
	LUMP_LIGHTING					= 8,	// *
	LUMP_OCCLUSION					= 9,
	LUMP_LEAFS						= 10,	// *
	LUMP_FACEIDS					= 11,
	LUMP_EDGES						= 12,	// *
	LUMP_SURFEDGES					= 13,	// *
	LUMP_MODELS						= 14,	// *
	LUMP_WORLDLIGHTS				= 15,	// 
	LUMP_LEAFFACES					= 16,	// *
	LUMP_LEAFBRUSHES				= 17,	// *
	LUMP_BRUSHES					= 18,	// *
	LUMP_BRUSHSIDES					= 19,	// *
	LUMP_AREAS						= 20,	// *
	LUMP_AREAPORTALS				= 21,	// *
	LUMP_UNUSED0					= 22,
	LUMP_UNUSED1					= 23,
	LUMP_UNUSED2					= 24,
	LUMP_UNUSED3					= 25,
	LUMP_DISPINFO					= 26,
	LUMP_ORIGINALFACES				= 27,
	LUMP_PHYSDISP					= 28,
	LUMP_PHYSCOLLIDE				= 29,
	LUMP_VERTNORMALS				= 30,
	LUMP_VERTNORMALINDICES			= 31,
	LUMP_DISP_LIGHTMAP_ALPHAS		= 32,
	LUMP_DISP_VERTS					= 33,		// CDispVerts
	LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,	// For each displacement
												//     For each lightmap sample
												//         byte for index
												//         if 255, then index = next byte + 255
												//         3 bytes for barycentric coordinates
	// The game lump is a method of adding game-specific lumps
	// FIXME: Eventually, all lumps could use the game lump system
	LUMP_GAME_LUMP					= 35,
	LUMP_LEAFWATERDATA				= 36,
	LUMP_PRIMITIVES					= 37,
	LUMP_PRIMVERTS					= 38,
	LUMP_PRIMINDICES				= 39,
	// A pak file can be embedded in a .bsp now, and the file system will search the pak
	//  file first for any referenced names, before deferring to the game directory 
	//  file system/pak files and finally the base directory file system/pak files.
	LUMP_PAKFILE					= 40,
	LUMP_CLIPPORTALVERTS			= 41,
	// A map can have a number of cubemap entities in it which cause cubemap renders
	// to be taken after running vrad.
	LUMP_CUBEMAPS					= 42,
	LUMP_TEXDATA_STRING_DATA		= 43,
	LUMP_TEXDATA_STRING_TABLE		= 44,
	LUMP_OVERLAYS					= 45,
	LUMP_LEAFMINDISTTOWATER			= 46,
	LUMP_FACE_MACRO_TEXTURE_INFO	= 47,
	LUMP_DISP_TRIS					= 48,
	LUMP_PHYSCOLLIDESURFACE			= 49,	// deprecated.  We no longer use win32-specific havok compression on terrain
	LUMP_WATEROVERLAYS              = 50,
	LUMP_LEAF_AMBIENT_INDEX_HDR		= 51,	// index of LUMP_LEAF_AMBIENT_LIGHTING_HDR
	LUMP_LEAF_AMBIENT_INDEX         = 52,	// index of LUMP_LEAF_AMBIENT_LIGHTING

	// optional lumps for HDR
	LUMP_LIGHTING_HDR				= 53,
	LUMP_WORLDLIGHTS_HDR			= 54,
	LUMP_LEAF_AMBIENT_LIGHTING_HDR	= 55,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.
	LUMP_LEAF_AMBIENT_LIGHTING		= 56,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.

	LUMP_XZIPPAKFILE				= 57,   // deprecated. xbox 1: xzip version of pak file
	LUMP_FACES_HDR					= 58,	// HDR maps may have different face data.
	LUMP_MAP_FLAGS                  = 59,   // extended level-wide flags. not present in all levels
	LUMP_OVERLAY_FADES				= 60,	// Fade distances for overlays
};

#define CSO2_LUMP_COMPRESSED 0x10000

struct lump_t
{
	int		fileofs, filelen;
	int		version;		// default to zero
							// this field was char fourCC[4] previously, but was unused, favoring the LUMP IDs above instead. It has been
							// repurposed for compression.  0 implies the lump is not compressed.
	int		uncompressedSize; // default to zero
};

struct dheader_t
{
	int			ident;
	int			version;
	lump_t		lumps[HEADER_LUMPS];
	int			mapRevision;				// the map's revision (iteration, version) number (added BSPVERSION 6)
};		

struct dgamelumpheader_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int lumpCount;

	// dgamelump_t follow this
};

// This is expected to be a four-CC code ('lump')
typedef int GameLumpId_t;

// game lump is compressed, filelen reflects original size
// use next entry fileofs to determine actual disk lump compressed size
// compression stage ensures a terminal null dictionary entry
#define GAMELUMPFLAG_COMPRESSED	0x0001

struct dgamelump_t
{
	//DECLARE_BYTESWAP_DATADESC();
	GameLumpId_t	id;
	unsigned short	flags;
	unsigned short	version;
	int				fileofs;
	int				filelen;
};

#define	MAXLIGHTMAPS	4

struct dface_t
{
	unsigned short	planenum;
	byte		side;	// faces opposite to the node's plane direction
	byte		onNode; // 1 of on node, 0 if in leaf

	int			firstedge;		// we must support > 64k edges
	short		numedges;
	short		texinfo;
	// This is a union under the assumption that a fog volume boundary (ie. water surface)
	// isn't a displacement map.
	// FIXME: These should be made a union with a flags or type field for which one it is
	// if we can add more to this.
	//	union
	//	{
	short       dispinfo;
	// This is only for surfaces that are the boundaries of fog volumes
	// (ie. water surfaces)
	// All of the rest of the surfaces can look at their leaf to find out
	// what fog volume they are in.
	short		surfaceFogVolumeID;
	//	};

	// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
	float       area;

	// TODO: make these unsigned chars?
	int			m_LightmapTextureMinsInLuxels[2];
	int			m_LightmapTextureSizeInLuxels[2];

	int origFace;				// reference the original face this face was derived from

	// non-polygon primitives (strips and lists)
//private:
	unsigned short m_NumPrims;	// Top bit, if set, disables shadows on this surface (this is why there are accessors).

public:
	unsigned short	firstPrimID;

	unsigned int	smoothingGroups;
};

struct dedge_t
{
	//DECLARE_BYTESWAP_DATADESC();
	unsigned short	v[2];		// vertex numbers
};

struct dnode_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
	short			area;		// If all leaves below this node are in the same area, then
								// this is the area index. If not, this is -1.
};

typedef struct texinfo_s
{
	//DECLARE_BYTESWAP_DATADESC();
	float		textureVecsTexelsPerWorldUnits[2][4];			// [s/t][xyz offset]
	float		lightmapVecsLuxelsPerWorldUnits[2][4];			// [s/t][xyz offset] - length is in units of texels/area
	int			flags;				// miptex flags + overrides
	int			texdata;			// Pointer to texture name, size, etc.
} texinfo_t;

#define TEXTURE_NAME_LENGTH	 128			// changed from 64 BSPVERSION 8

struct dtexdata_t
{
	//DECLARE_BYTESWAP_DATADESC();
	Vector		reflectivity;
	int			nameStringTableID;				// index into g_StringTable for the texture name
	int			width, height;					// source image
	int			view_width, view_height;		//
};

// helps get the offset of a bitfield
#define BEGIN_BITFIELD( name ) \
	union \
	{ \
		char name; \
		struct \
		{

#define END_BITFIELD() \
		}; \
	};

struct CompressedLightCube
{
	//DECLARE_BYTESWAP_DATADESC();
	ColorRGBExp32 m_Color[6];
};

struct dleaf_version_0_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;

	BEGIN_BITFIELD( bf );
	short			area:9;
	short			flags:7;			// Per leaf flags.
	END_BITFIELD();

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	short			leafWaterDataID; // -1 for not in water

	// Precaculated light info for entities.
	CompressedLightCube m_AmbientLighting;
};

// version 1
struct dleaf_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;

	BEGIN_BITFIELD( bf );
	short			area : 9;
	short			flags : 7;			// Per leaf flags.
	END_BITFIELD();

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	short			leafWaterDataID; // -1 for not in water

									 // NOTE: removed this for version 1 and moved into separate lump "LUMP_LEAF_AMBIENT_LIGHTING" or "LUMP_LEAF_AMBIENT_LIGHTING_HDR"
									 // Precaculated light info for entities.
	//	CompressedLightCube m_AmbientLighting;
};

struct dbrushside_t
{
	//DECLARE_BYTESWAP_DATADESC();
	unsigned short	planenum;		// facing out of the leaf
	short			texinfo;
	short			dispinfo;		// displacement info (BSPVERSION 7)
	short			bevel;			// is the side a bevel plane? (BSPVERSION 7)
};

struct dbrush_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int			firstside;
	int			numsides;
	int			contents;
};

struct dprimitive_t
{
	//DECLARE_BYTESWAP_DATADESC();
	unsigned char type;
	unsigned short	firstIndex;
	unsigned short	indexCount;
	unsigned short	firstVert;
	unsigned short	vertCount;
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct dareaportal_t
{
	//DECLARE_BYTESWAP_DATADESC();
	unsigned short	m_PortalKey;		// Entities have a key called portalnumber (and in vbsp a variable
										// called areaportalnum) which is used
										// to bind them to the area portals by comparing with this value.

	unsigned short	otherarea;		// The area this portal looks into.

	unsigned short	m_FirstClipPortalVert;	// Portal geometry.
	unsigned short	m_nClipPortalVerts;

	int				planenum;
};

struct dleafwaterdata_t
{
	//DECLARE_BYTESWAP_DATADESC();
	float	surfaceZ;
	float	minZ;
	short	surfaceTexInfoID;
};

#define OVERLAY_BSP_FACE_COUNT	64

#define OVERLAY_NUM_RENDER_ORDERS		(1<<OVERLAY_RENDER_ORDER_NUM_BITS)
#define OVERLAY_RENDER_ORDER_NUM_BITS	2
#define OVERLAY_RENDER_ORDER_MASK		0xC000	// top 2 bits set

struct doverlay_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int			nId;
	short		nTexInfo;

	// Accessors..
	/*void			SetFaceCount( unsigned short count );
	unsigned short	GetFaceCount() const;

	void			SetRenderOrder( unsigned short order );
	unsigned short	GetRenderOrder() const;

private:*/
	unsigned short	m_nFaceCountAndRenderOrder;

public:
	int			aFaces[OVERLAY_BSP_FACE_COUNT];
	float		flU[2];
	float		flV[2];
	Vector		vecUVPoints[4];
	Vector		vecOrigin;
	Vector		vecBasisNormal;
};

#define WATEROVERLAY_BSP_FACE_COUNT				256
#define WATEROVERLAY_RENDER_ORDER_NUM_BITS		2
#define WATEROVERLAY_NUM_RENDER_ORDERS			(1<<WATEROVERLAY_RENDER_ORDER_NUM_BITS)
#define WATEROVERLAY_RENDER_ORDER_MASK			0xC000	// top 2 bits set
struct dwateroverlay_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int				nId;
	short			nTexInfo;

	// Accessors..
	/*void			SetFaceCount( unsigned short count );
	unsigned short	GetFaceCount() const;
	void			SetRenderOrder( unsigned short order );
	unsigned short	GetRenderOrder() const;

private:*/

	unsigned short	m_nFaceCountAndRenderOrder;

public:

	int				aFaces[WATEROVERLAY_BSP_FACE_COUNT];
	float			flU[2];
	float			flV[2];
	Vector			vecUVPoints[4];
	Vector			vecOrigin;
	Vector			vecBasisNormal;
};

// lights that were used to illuminate the world
enum emittype_t
{
	emit_surface,		// 90 degree spotlight
	emit_point,			// simple point light source
	emit_spotlight,		// spotlight with penumbra
	emit_skylight,		// directional light with no falloff (surface must trace to SKY texture)
	emit_quakelight,	// linear falloff, non-lambertian
	emit_skyambient,	// spherical light source with no falloff (surface must trace to SKY texture)
};


// Flags for dworldlight_t::flags
#define DWL_FLAGS_INAMBIENTCUBE		0x0001	// This says that the light was put into the per-leaf ambient cubes.


struct dworldlight_t
{
	//DECLARE_BYTESWAP_DATADESC();
	Vector		origin;
	Vector		intensity;
	Vector		normal;			// for surfaces and spotlights
	int			cluster;
	emittype_t	type;
	int			style;
	float		stopdot;		// start of penumbra for emit_spotlight
	float		stopdot2;		// end of penumbra for emit_spotlight
	float		exponent;		// 
	float		radius;			// cutoff distance
								// falloff for emit_spotlight + emit_point: 
								// 1 / (constant_attn + linear_attn * dist + quadratic_attn * dist^2)
	float		constant_attn;
	float		linear_attn;
	float		quadratic_attn;
	int			flags;			// Uses a combination of the DWL_FLAGS_ defines.
	int			texinfo;		// 
	int			owner;			// entity that this light it relative to
};

struct dcubemapsample_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int			origin[3];			// position of light snapped to the nearest integer
									// the filename for the vtf file is derived from the position
	unsigned char size;				// 0 - default
									// otherwise, 1<<(size-1)
};

// from gamebspfile.h

enum
{
	GAMELUMP_DETAIL_PROPS = 'dprp',
	GAMELUMP_DETAIL_PROP_LIGHTING = 'dplt',
	GAMELUMP_STATIC_PROPS = 'sprp',
	GAMELUMP_DETAIL_PROP_LIGHTING_HDR = 'dplh',
};

//-----------------------------------------------------------------------------
// This is the data associated with the GAMELUMP_STATIC_PROPS lump
//-----------------------------------------------------------------------------
enum
{
	STATIC_PROP_NAME_LENGTH  = 128,

	// Flags field
	// These are automatically computed
	STATIC_PROP_FLAG_FADES	= 0x1,
	STATIC_PROP_USE_LIGHTING_ORIGIN	= 0x2,
	STATIC_PROP_NO_DRAW = 0x4,	// computed at run time based on dx level

	// These are set in WC
	STATIC_PROP_IGNORE_NORMALS	= 0x8,
	STATIC_PROP_NO_SHADOW	= 0x10,
	STATIC_PROP_SCREEN_SPACE_FADE	= 0x20,

	STATIC_PROP_NO_PER_VERTEX_LIGHTING = 0x40,				// in vrad, compute lighting at
															// lighting origin, not for each vertex
	
	STATIC_PROP_NO_SELF_SHADOWING = 0x80,					// disable self shadowing in vrad

	STATIC_PROP_NO_PER_TEXEL_LIGHTING = 0x100,				// whether we should do per-texel lightmaps in vrad.

	STATIC_PROP_WC_MASK		= 0x1d8,						// all flags settable in hammer (?)
};

struct StaticPropDictLump_t
{
	//DECLARE_BYTESWAP_DATADESC();
	char	m_Name[STATIC_PROP_NAME_LENGTH];		// model name
};

struct StaticPropLeafLump_t
{
	//DECLARE_BYTESWAP_DATADESC();
	unsigned short	m_Leaf;
};

struct StaticPropLumpV6_t
{
	//DECLARE_BYTESWAP_DATADESC();
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_PropType;
	unsigned short	m_FirstLeaf;
	unsigned short	m_LeafCount;
	unsigned char	m_Solid;
	unsigned char	m_Flags;
	int				m_Skin;
	float			m_FadeMinDist;
	float			m_FadeMaxDist;
	Vector			m_LightingOrigin;
	float			m_flForcedFadeScale;
	unsigned short	m_nMinDXLevel;
	unsigned short	m_nMaxDXLevel;
	//	int				m_Lighting;			// index into the GAMELUMP_STATIC_PROP_LIGHTING lump
};

#define DETAIL_NAME_LENGTH 128

//-----------------------------------------------------------------------------
// Model index when using studiomdls for detail props
//-----------------------------------------------------------------------------
struct DetailObjectDictLump_t
{
	//DECLARE_BYTESWAP_DATADESC();
	char	m_Name[DETAIL_NAME_LENGTH];		// model name
};

//-----------------------------------------------------------------------------
// Information about the sprite to render
//-----------------------------------------------------------------------------
struct DetailSpriteDictLump_t
{
	//DECLARE_BYTESWAP_DATADESC();
	// NOTE: All detail prop sprites must lie in the material detail/detailsprites
	Vector2D	m_UL;		// Coordinate of upper left 
	Vector2D	m_LR;		// Coordinate of lower right
	Vector2D	m_TexUL;	// Texcoords of upper left
	Vector2D	m_TexLR;	// Texcoords of lower left
};

struct DetailObjectLump_t
{
	//DECLARE_BYTESWAP_DATADESC();
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_DetailModel;		// either index into DetailObjectDictLump_t or DetailPropSpriteLump_t
	unsigned short	m_Leaf;
	ColorRGBExp32	m_Lighting;
	unsigned int	m_LightStyles;
	unsigned char	m_LightStyleCount;
	unsigned char   m_SwayAmount;		// how much do the details sway
	unsigned char	m_ShapeAngle;		// angle param for shaped sprites
	unsigned char   m_ShapeSize;		// size param for shaped sprites
	unsigned char	m_Orientation;		// See DetailPropOrientation_t
	unsigned char	m_Padding2[3];		// FIXME: Remove when we rev the detail lump again..
	unsigned char	m_Type;				// See DetailPropType_t
	unsigned char	m_Padding3[3];		// FIXME: Remove when we rev the detail lump again..
	float			m_flScale;			// For sprites only currently
};

//========================================================================//
// end of copyrighted code
//========================================================================//

struct cso2dface_t
{
	unsigned long	planenum;
	byte		side;	// faces opposite to the node's plane direction
	byte		onNode; // 1 of on node, 0 if in leaf

	int			firstedge;		// we must support > 64k edges
	long		numedges;
	long		texinfo;
	// This is a union under the assumption that a fog volume boundary (ie. water surface)
	// isn't a displacement map.
	// FIXME: These should be made a union with a flags or type field for which one it is
	// if we can add more to this.
	//	union
	//	{
	long       dispinfo;
	long		surfaceFogVolumeID;
	//	};

	// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
	float       area;

	// TODO: make these unsigned chars?
	int			m_LightmapTextureMinsInLuxels[2];
	int			m_LightmapTextureSizeInLuxels[2];

	int origFace;				// reference the original face this face was derived from

								// non-polygon primitives (strips and lists)
	unsigned long m_NumPrims;	// Top bit, if set, disables shadows on this surface (this is why there are accessors).

	unsigned long	firstPrimID;

	unsigned int	smoothingGroups;
};

struct cso2dedge_t
{
	//DECLARE_BYTESWAP_DATADESC();
	unsigned long	v[2];		// vertex numbers
};

struct cso2dnode_t
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	long		mins[3];		// for frustom culling
	long		maxs[3];
	unsigned long	firstface;
	unsigned long	numfaces;	// counting both sides
	long			area;		// If all leaves below this node are in the same area, then
								// this is the area index. If not, this is -1.
};

struct cso2dleaf_version_0_t
{
	int				contents;			// OR of all brushes (not needed?)

	long			cluster;

	BEGIN_BITFIELD( bf );
	short			area : 9;
	short			flags : 7;			// Per leaf flags.
	END_BITFIELD();

	long			mins[3];			// for frustum culling
	long			maxs[3];

	unsigned long	firstleafface;
	unsigned long	numleaffaces;

	unsigned long	firstleafbrush;
	unsigned long	numleafbrushes;
	long			leafWaterDataID; // -1 for not in water

	// Precaculated light info for entities.
	CompressedLightCube m_AmbientLighting;
};

struct cso2dleaf_t
{
	int				contents;			// OR of all brushes (not needed?)

	long			cluster;

	BEGIN_BITFIELD( bf );
	short			area : 9;
	short			flags : 7;			// Per leaf flags.
	END_BITFIELD();

	long			mins[3];			// for frustum culling
	long			maxs[3];

	unsigned long	firstleafface;
	unsigned long	numleaffaces;

	unsigned long	firstleafbrush;
	unsigned long	numleafbrushes;
	long			leafWaterDataID; // -1 for not in water

	// Precaculated light info for entities.
	//CompressedLightCube m_AmbientLighting;
};

struct cso2dbrushside_t
{
	unsigned long	planenum;		// facing out of the leaf
	long			texinfo;
	short			dispinfo;		// displacement info (BSPVERSION 7)
	short			bevel;			// is the side a bevel plane? (BSPVERSION 7)
};

struct cso2dprimitive_t
{
	unsigned long	type;
	unsigned long	firstIndex;
	unsigned long	indexCount;
	unsigned long	firstVert;
	unsigned long	vertCount;
};

struct cso2dareaportal_t
{
	unsigned long	m_PortalKey;		// Entities have a key called portalnumber (and in vbsp a variable
										// called areaportalnum) which is used
										// to bind them to the area portals by comparing with this value.

	unsigned long	otherarea;		// The area this portal looks into.

	unsigned long	m_FirstClipPortalVert;	// Portal geometry.
	unsigned long	m_nClipPortalVerts;

	int				planenum;
};

struct cso2dleafwaterdata_t
{
	float	surfaceZ;
	float	minZ;
	long	surfaceTexInfoID;
};

struct cso2doverlay_t
{
	int			nId;
	long		nTexInfo;
	unsigned long	m_nFaceCountAndRenderOrder;
	int			aFaces[OVERLAY_BSP_FACE_COUNT];
	float		flU[2];
	float		flV[2];
	Vector		vecUVPoints[4];
	Vector		vecOrigin;
	Vector		vecBasisNormal;
};

struct cso2dwateroverlay_t
{
	int				nId;
	long			nTexInfo;
	unsigned long	m_nFaceCountAndRenderOrder;
	int				aFaces[WATEROVERLAY_BSP_FACE_COUNT];
	float			flU[2];
	float			flV[2];
	Vector			vecUVPoints[4];
	Vector			vecOrigin;
	Vector			vecBasisNormal;
};

struct cso2dcubemapsample_t
{
	int			origin[3];			// position of light snapped to the nearest integer
									// the filename for the vtf file is derived from the position
	unsigned char size;				// 0 - default
									// otherwise, 1<<(size-1)
	uint8_t pad2[151]; // aim_dust gets 0xA0, but game's sizeof is 0xA4??
};

struct CSO2StaticPropLeafLump_t
{
	unsigned long	m_Leaf;
};

struct CSO2StaticPropLumpV6_t
{
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_PropType;
	unsigned long	m_FirstLeaf;
	unsigned long	m_LeafCount;
	unsigned char	m_Solid;
	unsigned char	m_Flags;
	int				m_Skin;
	float			m_FadeMinDist;
	float			m_FadeMaxDist;
	Vector			m_LightingOrigin;
	float			m_flForcedFadeScale;
	unsigned short	m_nMinDXLevel;
	unsigned short	m_nMaxDXLevel;
	//	int				m_Lighting;			// index into the GAMELUMP_STATIC_PROP_LIGHTING lump
};

struct CSO2StaticPropLump_t
{
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_PropType;
	unsigned long	m_FirstLeaf;
	unsigned long	m_LeafCount;
	unsigned char	m_Solid;
	unsigned char	m_Flags;
	int				m_Skin;
	float			m_FadeMinDist;
	float			m_FadeMaxDist;
	Vector			m_LightingOrigin;
	float			m_flForcedFadeScale;
	unsigned short	m_nMinDXLevel;
	unsigned short	m_nMaxDXLevel;
	//	int				m_Lighting;			// index into the GAMELUMP_STATIC_PROP_LIGHTING lump
	char pad[112]; // cso2's custom static prop lump
};

struct CSO2DetailObjectLump_t
{
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_DetailModel;		// either index into DetailObjectDictLump_t or DetailPropSpriteLump_t
	unsigned long	m_Leaf;
	ColorRGBExp32	m_Lighting;
	unsigned int	m_LightStyles;
	unsigned char	m_LightStyleCount;
	unsigned char   m_SwayAmount;		// how much do the details sway
	unsigned char	m_ShapeAngle;		// angle param for shaped sprites
	unsigned char   m_ShapeSize;		// size param for shaped sprites
	unsigned char	m_Orientation;		// See DetailPropOrientation_t
	unsigned char	m_Padding2[3];		// FIXME: Remove when we rev the detail lump again..
	unsigned char	m_Type;				// See DetailPropType_t
	unsigned char	m_Padding3[3];		// FIXME: Remove when we rev the detail lump again..
	float			m_flScale;			// For sprites only currently
};

bool IsBspFile( uint8_t* pBuffer );
bool DecompressBsp( uint8_t*& pBuffer, uint32_t& iBufferSize, bool bShouldFixLumps );
