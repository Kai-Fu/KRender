#include "tile2d.h"

Tile2DSet::Tile2DSet()
{
	mWidth = 0;
	mHeight = 0;
	mTileSize = 0;

	mGridX = 0;
	mGridY = 0;
	mCurGrid = 0;
}

Tile2DSet::~Tile2DSet()
{

}

void Tile2DSet::Reset(UINT32 w, UINT32 h, UINT32 tile_size)
{
	mWidth = w;
	mHeight = h;
	mTileSize = tile_size;

	mGridX = mWidth / tile_size;
	if (0 != (mWidth % tile_size)) ++mGridX;
	mGridY = mHeight / tile_size;
	if (0 != (mHeight % tile_size)) ++mGridY;

	mCurGrid = 0;
	mMaxGridIndex = mGridX * mGridY;
}

bool Tile2DSet::GetNextTile(TileDesc& desc)
{
	long idx = atomic_increment(&mCurGrid) - 1;

	if (idx >= mMaxGridIndex)
		return false;

	long gy = idx / mGridX;
	long gx = idx % mGridX;
	desc.grid_x = gx;
	desc.grid_y = gy;
	desc.start_x = gx * mTileSize;
	desc.start_y = gy * mTileSize;
	desc.tile_w = std::min(mWidth - desc.start_x, mTileSize);
	desc.tile_h = std::min(mHeight - desc.start_y, mTileSize);
	return true;
}