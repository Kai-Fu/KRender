#pragma once
#include "../base/base_header.h"
#include "../os/api_wrapper.h"

class Tile2DSet
{
public:
	Tile2DSet();
	~Tile2DSet();

	void Reset(UINT32 w, UINT32 h, UINT32 tile_size);
	
	struct TileDesc {
		UINT32 start_x;
		UINT32 start_y;
		UINT32 tile_w;
		UINT32 tile_h;
		UINT32 grid_x;
		UINT32 grid_y;
	};
	bool GetNextTile(TileDesc& desc);

protected:
	UINT32 mWidth;
	UINT32 mHeight;
	UINT32 mTileSize;

	UINT32 mGridX;
	UINT32 mGridY;

	LOCK_FREE_LONG mCurGrid;
	long mMaxGridIndex;
};