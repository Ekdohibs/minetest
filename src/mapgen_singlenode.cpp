/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "mapgen_singlenode.h"
#include "voxel.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
#include "profiler.h"
#include "emerge.h"

//////////////////////// Mapgen Singlenode parameter read/write

bool MapgenSinglenodeParams::readParams(Settings *settings) {
	return true;
}


void MapgenSinglenodeParams::writeParams(Settings *settings) {
}

///////////////////////////////////////////////////////////////////////////////

MapgenSinglenode::MapgenSinglenode(int mapgenid, MapgenSinglenodeParams *params) {
	flags = params->flags;
}


MapgenSinglenode::~MapgenSinglenode() {
}

//////////////////////// Map generator

void MapgenSinglenode::makeChunk(BlockMakeData *data) {
	assert(data->vmanip);
	assert(data->nodedef);
	assert(data->blockpos_requested.X >= data->blockpos_min.X &&
		   data->blockpos_requested.Y >= data->blockpos_min.Y &&
		   data->blockpos_requested.Z >= data->blockpos_min.Z);
	assert(data->blockpos_requested.X <= data->blockpos_max.X &&
		   data->blockpos_requested.Y <= data->blockpos_max.Y &&
		   data->blockpos_requested.Z <= data->blockpos_max.Z);

	this->generating = true;
	this->vm   = data->vmanip;	
	this->ndef = data->nodedef;
			
	v3POS blockpos_min = data->blockpos_min;
	v3POS blockpos_max = data->blockpos_max;

	// Area of central chunk
	v3POS node_min = blockpos_min*MAP_BLOCKSIZE;
	v3POS node_max = (blockpos_max+v3POS(1,1,1))*MAP_BLOCKSIZE-v3POS(1,1,1);

	content_t c_node = ndef->getId("mapgen_singlenode");
	if (c_node == CONTENT_IGNORE)
		c_node = CONTENT_AIR;
	
	MapNode n_node(c_node);
	
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y; y <= node_max.Y; y++) {
		u32 i = vm->m_area.index(node_min.X, y, z);
		for (s16 x = node_min.X; x <= node_max.X; x++) {
			if (vm->m_data[i].getContent() == CONTENT_IGNORE)
				vm->m_data[i] = n_node;
			i++;
		}
	}

	// Add top and bottom side of water to transforming_liquid queue
	updateLiquid(&data->transforming_liquid, node_min, node_max);

	// Calculate lighting
	if (!(flags & MG_NOLIGHT))
		calcLighting(node_min - v3POS(1, 0, 1) * MAP_BLOCKSIZE,
					 node_max + v3POS(1, 0, 1) * MAP_BLOCKSIZE);
	
	this->generating = false;
}

int MapgenSinglenode::getGroundLevelAtPoint(v2POS p) {
	return 0;
}

