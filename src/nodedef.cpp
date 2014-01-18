/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "nodedef.h"

#include "main.h" // For g_settings
#include "itemdef.h"
#ifndef SERVER
#include "tile.h"
#endif
#include "log.h"
#include "settings.h"
#include "nameidmapping.h"
#include "util/numeric.h"
#include "util/serialize.h"
//#include "profiler.h" // For TimeTaker
#include "util/template_serialize.h"

/*
	NodeBox
*/

void NodeBox::reset()
{
	type = NODEBOX_REGULAR;
	// default is empty
	fixed.clear();
	// default is sign/ladder-like
	wall_top = aabb3f(-BS/2, BS/2-BS/16., -BS/2, BS/2, BS/2, BS/2);
	wall_bottom = aabb3f(-BS/2, -BS/2, -BS/2, BS/2, -BS/2+BS/16., BS/2);
	wall_side = aabb3f(-BS/2, -BS/2, -BS/2, -BS/2+BS/16., BS/2, BS/2);
}

void NodeBox::serialize(std::ostream &os, u16 protocol_version) const
{
	int version = protocol_version >= 21 ? 2 : 1;
	writeU8(os, version);

	if (version == 1 && type == NODEBOX_LEVELED)
		writeU8(os, NODEBOX_FIXED);
	else
		writeU8(os, type);

	if(type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
	{
		writeU16(os, fixed.size());
		for(std::vector<aabb3f>::const_iterator
				i = fixed.begin();
				i != fixed.end(); i++)
		{
			writeV3F1000(os, i->MinEdge);
			writeV3F1000(os, i->MaxEdge);
		}
	}
	else if(type == NODEBOX_WALLMOUNTED)
	{
		writeV3F1000(os, wall_top.MinEdge);
		writeV3F1000(os, wall_top.MaxEdge);
		writeV3F1000(os, wall_bottom.MinEdge);
		writeV3F1000(os, wall_bottom.MaxEdge);
		writeV3F1000(os, wall_side.MinEdge);
		writeV3F1000(os, wall_side.MaxEdge);
	}
}

void NodeBox::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version < 1 || version > 2)
		throw SerializationError("unsupported NodeBox version");

	reset();

	type = (enum NodeBoxType)readU8(is);

	if(type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
	{
		u16 fixed_count = readU16(is);
		while(fixed_count--)
		{
			aabb3f box;
			box.MinEdge = readV3F1000(is);
			box.MaxEdge = readV3F1000(is);
			fixed.push_back(box);
		}
	}
	else if(type == NODEBOX_WALLMOUNTED)
	{
		wall_top.MinEdge = readV3F1000(is);
		wall_top.MaxEdge = readV3F1000(is);
		wall_bottom.MinEdge = readV3F1000(is);
		wall_bottom.MaxEdge = readV3F1000(is);
		wall_side.MinEdge = readV3F1000(is);
		wall_side.MaxEdge = readV3F1000(is);
	}
}

template<>
struct STraits<NodeBox>
{
	static u8 type()
	{
		return VALUE_TYPE_RAW;
	}
	static bool read(const std::vector<u8> &src, NodeBox *result)
	{
		if(result){
			std::istringstream is(std::string((char*)&src[0], src.size()),
					std::ios::binary);
			result->deSerialize(is);
		}
		return true;
	}
	static void write(const NodeBox &src, std::vector<u8> &result, u16 protocol_version)
	{
		std::ostringstream os(std::ios::binary);
		src.serialize(os, protocol_version);
		std::string s = os.str();
		result.insert(result.begin(), (u8*)s.c_str(),
				(u8*)s.c_str() + s.size());
	}
};

/*
	TileDef
*/

void TileDef::serialize(std::ostream &os, u16 protocol_version) const
{
	if(protocol_version >= 17)
		writeU8(os, 1); 
	else
		writeU8(os, 0);
	os<<serializeString(name);
	writeU8(os, animation.type);
	writeU16(os, animation.aspect_w);
	writeU16(os, animation.aspect_h);
	writeF1000(os, animation.length);
	if(protocol_version >= 17)
		writeU8(os, backface_culling);
}

void TileDef::deSerialize(std::istream &is)
{
	int version = readU8(is);
	name = deSerializeString(is);
	animation.type = (TileAnimationType)readU8(is);
	animation.aspect_w = readU16(is);
	animation.aspect_h = readU16(is);
	animation.length = readF1000(is);
	if(version >= 1)
		backface_culling = readU8(is);
}

template<>
struct STraits<TileDef>
{
	static u8 type()
	{
		return VALUE_TYPE_RAW;
	}
	static bool read(const std::vector<u8> &src, TileDef *result)
	{
		if(result){
			std::istringstream is(std::string((char*)&src[0], src.size()),
					std::ios::binary);
			result->deSerialize(is);
		}
		return true;
	}
	static void write(const TileDef &src, std::vector<u8> &result, u16 protocol_version)
	{
		std::ostringstream os(std::ios::binary);
		src.serialize(os, protocol_version);
		std::string s = os.str();
		result.insert(result.begin(), (u8*)s.c_str(),
				(u8*)s.c_str() + s.size());
	}
};

/*
	SimpleSoundSpec serialization
*/

static void serializeSimpleSoundSpec(const SimpleSoundSpec &ss,
		std::ostream &os)
{
	os<<serializeString(ss.name);
	writeF1000(os, ss.gain);
}
static void deSerializeSimpleSoundSpec(SimpleSoundSpec &ss, std::istream &is)
{
	ss.name = deSerializeString(is);
	ss.gain = readF1000(is);
}

template<>
struct STraits<SimpleSoundSpec>
{
	static u8 type()
	{
		return VALUE_TYPE_RAW;
	}
	static bool read(const std::vector<u8> &src, SimpleSoundSpec *result)
	{
		if(result){
			std::istringstream is(std::string((char*)&src[0], src.size()),
					std::ios::binary);
			deSerializeSimpleSoundSpec(*result, is);
		}
		return true;
	}
	static void write(const SimpleSoundSpec &src, std::vector<u8> &result, u16 protocol_version)
	{
		std::ostringstream os(std::ios::binary);
		serializeSimpleSoundSpec(src, os);
		std::string s = os.str();
		result.insert(result.begin(), (u8*)s.c_str(),
				(u8*)s.c_str() + s.size());
	}
};

/*
	ContentFeatures
*/

ContentFeatures::ContentFeatures()
{
	reset();
}

ContentFeatures::~ContentFeatures()
{
}

void ContentFeatures::reset()
{
	/*
		Cached stuff
	*/
#ifndef SERVER
	solidness = 2;
	visual_solidness = 0;
	backface_culling = true;
#endif
	has_on_construct = false;
	has_on_destruct = false;
	has_after_destruct = false;
	/*
		Actual data

		NOTE: Most of this is always overridden by the default values given
		      in builtin.lua
	*/
	name = "";
	groups.clear();
	// Unknown nodes can be dug
	groups["dig_immediate"] = 2;
	drawtype = NDT_NORMAL;
	visual_scale = 1.0;
	for(u32 i=0; i<6; i++)
		tiledef[i] = TileDef();
	for(u16 j=0; j<CF_SPECIAL_COUNT; j++)
		tiledef_special[j] = TileDef();
	alpha = 255;
	post_effect_color = video::SColor(0, 0, 0, 0);
	param_type = CPT_NONE;
	param_type_2 = CPT2_NONE;
	is_ground_content = false;
	light_propagates = false;
	sunlight_propagates = false;
	walkable = true;
	pointable = true;
	diggable = true;
	climbable = false;
	buildable_to = false;
	rightclickable = true;
	leveled = 0;
	liquid_type = LIQUID_NONE;
	liquid_alternative_flowing = "";
	liquid_alternative_source = "";
	liquid_viscosity = 0;
	liquid_renewable = true;
	freezemelt = "";
	liquid_range = LIQUID_LEVEL_MAX+1;
	drowning = 0;
	light_source = 0;
	damage_per_second = 0;
	node_box = NodeBox();
	selection_box = NodeBox();
	waving = 0;
	legacy_facedir_simple = false;
	legacy_wallmounted = false;
	sound_footstep = SimpleSoundSpec();
	sound_dig = SimpleSoundSpec("__group");
	sound_dug = SimpleSoundSpec();
}

// NEVER remove anything from this list
// NEVER put anything in between of items in this list
// This enum is licensed under WTFPL.
enum{
	NODEDEF_NAME,
	NODEDEF_GROUP_NAMES,
	NODEDEF_GROUP_VALUES,
	NODEDEF_DRAWTYPE,
	NODEDEF_VISUAL_SCALE,
	NODEDEF_TILEDEFS,
	NODEDEF_TILEDEF_SPECIALS,
	NODEDEF_ALPHA,
	NODEDEF_POST_EFFECT_COLOR,
	NODEDEF_PARAM_TYPE,
	NODEDEF_PARAM_TYPE_2,
	NODEDEF_IS_GROUND_CONTENT,
	NODEDEF_LIGHT_PROPAGATES,
	NODEDEF_SUNLIGHT_PROPAGATES,
	NODEDEF_WALKABLE,
	NODEDEF_POINTABLE,
	NODEDEF_DIGGABLE,
	NODEDEF_CLIMBABLE,
	NODEDEF_BUILDABLE_TO,
	NODEDEF_LIQUID_TYPE,
	NODEDEF_LIQUID_ALTERNATIVE_FLOWING,
	NODEDEF_LIQUID_ALTERNATIVE_SOURCE,
	NODEDEF_LIQUID_VISCOSITY,
	NODEDEF_LIQUID_RENEWABLE,
	NODEDEF_LIGHT_SOURCE,
	NODEDEF_DAMAGE_PER_SECOND,
	NODEDEF_NODE_BOX,
	NODEDEF_SELECTION_BOX,
	NODEDEF_LEGACY_FACEDIR_SIMPLE,
	NODEDEF_LEGACY_WALLMOUNTED,
	NODEDEF_SOUND_FOOTSTEP,
	NODEDEF_SOUND_DIG,
	NODEDEF_SOUND_DUG,
	NODEDEF_RIGHTCLICKABLE,
	NODEDEF_DROWNING,
	NODEDEF_LEVELED,
	NODEDEF_LIQUID_RANGE,
	NODEDEF_WAVING,
};

void ContentFeatures::serialize(std::ostream &os, u16 protocol_version) const
{
	if(protocol_version < 22){ //TODO: Fix that number
		serializeOld(os, protocol_version);
		return;
	}

	writeU8(os, 7); // version
	BKVL bv;
	bv.append(NODEDEF_NAME, name);
	for(ItemGroupList::const_iterator
			i = groups.begin(); i != groups.end(); i++){
		bv.append(NODEDEF_GROUP_NAMES, i->first);
		bv.append<s16>(NODEDEF_GROUP_VALUES, i->second);
	}
	bv.append<u8>(NODEDEF_DRAWTYPE, drawtype);
	bv.append(NODEDEF_VISUAL_SCALE, F1000(visual_scale));
	for(u32 i=0; i<6; i++)
		bv.append(NODEDEF_TILEDEFS, tiledef[i], protocol_version);
	for(u32 i=0; i<CF_SPECIAL_COUNT; i++)
		bv.append(NODEDEF_TILEDEF_SPECIALS, tiledef_special[i], protocol_version);
	bv.append(NODEDEF_ALPHA, alpha);
	bv.append(NODEDEF_POST_EFFECT_COLOR, post_effect_color);
	bv.append<u8>(NODEDEF_PARAM_TYPE, param_type);
	bv.append<u8>(NODEDEF_PARAM_TYPE_2, param_type_2);
	bv.append(NODEDEF_IS_GROUND_CONTENT, is_ground_content);
	bv.append(NODEDEF_LIGHT_PROPAGATES, light_propagates);
	bv.append(NODEDEF_SUNLIGHT_PROPAGATES, sunlight_propagates);
	bv.append(NODEDEF_WALKABLE, walkable);
	bv.append(NODEDEF_POINTABLE, pointable);
	bv.append(NODEDEF_DIGGABLE, diggable);
	bv.append(NODEDEF_CLIMBABLE, climbable);
	bv.append(NODEDEF_BUILDABLE_TO, buildable_to);
	bv.append<u8>(NODEDEF_LIQUID_TYPE, liquid_type);
	bv.append(NODEDEF_LIQUID_ALTERNATIVE_FLOWING, liquid_alternative_flowing);
	bv.append(NODEDEF_LIQUID_ALTERNATIVE_SOURCE, liquid_alternative_source);
	bv.append(NODEDEF_LIQUID_VISCOSITY, liquid_viscosity);
	bv.append(NODEDEF_LIQUID_RENEWABLE, liquid_renewable);
	bv.append(NODEDEF_LIGHT_SOURCE, light_source);
	bv.append(NODEDEF_DAMAGE_PER_SECOND, damage_per_second);
	bv.append(NODEDEF_NODE_BOX, node_box, protocol_version);
	bv.append(NODEDEF_SELECTION_BOX, selection_box, protocol_version);
	bv.append(NODEDEF_LEGACY_FACEDIR_SIMPLE, legacy_facedir_simple);
	bv.append(NODEDEF_LEGACY_WALLMOUNTED, legacy_wallmounted);
	bv.append(NODEDEF_SOUND_FOOTSTEP, sound_footstep);
	bv.append(NODEDEF_SOUND_DIG, sound_dig);
	bv.append(NODEDEF_SOUND_DUG, sound_dug);
	bv.append(NODEDEF_RIGHTCLICKABLE, rightclickable);
	bv.append(NODEDEF_DROWNING, drowning);
	bv.append(NODEDEF_LEVELED, leveled);
	bv.append(NODEDEF_LIQUID_RANGE, liquid_range);
	bv.append(NODEDEF_WAVING, waving);
	
	bv.serialize(os);
}

void ContentFeatures::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 7){
		deSerializeOld(is, version);
		return;
	}

	reset();
	BKVL bv;
	bv.deSerialize(is);
	bv.get(NODEDEF_NAME, 0, name);
	for(u32 i=0; i<bv.valueCount(NODEDEF_GROUP_NAMES); i++){
		std::string name;
		if(!bv.get(NODEDEF_GROUP_NAMES, i, name))
			continue;
		groups[name] = bv.getDirect(NODEDEF_GROUP_VALUES, i, 1);
	}
	u8 drawtype_tmp;
	if(bv.get<u8>(NODEDEF_DRAWTYPE, 0, drawtype_tmp))
		drawtype = (NodeDrawType)drawtype_tmp;
	for(u32 i=0; i<bv.valueCount(NODEDEF_TILEDEFS) &&
			i < 6; i++){
		bv.get(NODEDEF_TILEDEFS, i, tiledef[i]);
	}
	for(u32 i=0; i<bv.valueCount(NODEDEF_TILEDEF_SPECIALS) &&
			i < CF_SPECIAL_COUNT; i++){
		bv.get(NODEDEF_TILEDEF_SPECIALS, i, tiledef_special[i]);
	}
	bv.get(NODEDEF_ALPHA, 0, alpha);
	bv.get(NODEDEF_POST_EFFECT_COLOR, 0, post_effect_color);
	u8 param_type_tmp;
	if(bv.get(NODEDEF_PARAM_TYPE, 0, param_type_tmp))
		param_type = (ContentParamType)param_type_tmp;
	u8 param_type_2_tmp;
	if(bv.get(NODEDEF_PARAM_TYPE_2, 0, param_type_2_tmp))
		param_type_2 = (ContentParamType2)param_type_2_tmp;
	bv.get(NODEDEF_IS_GROUND_CONTENT, 0, is_ground_content);
	bv.get(NODEDEF_LIGHT_PROPAGATES, 0, light_propagates);
	bv.get(NODEDEF_SUNLIGHT_PROPAGATES, 0, sunlight_propagates);
	bv.get(NODEDEF_WALKABLE, 0, walkable);
	bv.get(NODEDEF_POINTABLE, 0, pointable);
	bv.get(NODEDEF_DIGGABLE, 0, diggable);
	bv.get(NODEDEF_CLIMBABLE, 0, climbable);
	bv.get(NODEDEF_BUILDABLE_TO, 0, buildable_to);
	u8 liquid_type_tmp;
	if(bv.get(NODEDEF_LIQUID_TYPE, 0, liquid_type_tmp))
		liquid_type = (LiquidType)liquid_type_tmp;
	bv.get(NODEDEF_LIQUID_ALTERNATIVE_FLOWING, 0, liquid_alternative_flowing);
	bv.get(NODEDEF_LIQUID_ALTERNATIVE_SOURCE, 0, liquid_alternative_source);
	bv.get(NODEDEF_LIQUID_VISCOSITY, 0, liquid_viscosity);
	bv.get(NODEDEF_LIQUID_RENEWABLE, 0, liquid_renewable);
	bv.get(NODEDEF_LIGHT_SOURCE, 0, light_source);
	bv.get(NODEDEF_DAMAGE_PER_SECOND, 0, damage_per_second);
	bv.get(NODEDEF_NODE_BOX, 0, node_box);
	bv.get(NODEDEF_SELECTION_BOX, 0, selection_box);
	bv.get(NODEDEF_LEGACY_FACEDIR_SIMPLE, 0, legacy_facedir_simple);
	bv.get(NODEDEF_LEGACY_WALLMOUNTED, 0, legacy_wallmounted);
	bv.get(NODEDEF_SOUND_FOOTSTEP, 0, sound_footstep);
	bv.get(NODEDEF_SOUND_DIG, 0, sound_dig);
	bv.get(NODEDEF_SOUND_DUG, 0, sound_dug);
	bv.get(NODEDEF_RIGHTCLICKABLE, 0, rightclickable);
	bv.get(NODEDEF_DROWNING, 0, drowning);
	bv.get(NODEDEF_LEVELED, 0, leveled);
	bv.get(NODEDEF_LIQUID_RANGE, 0, liquid_range);
	bv.get(NODEDEF_WAVING, 0, waving);
}

/*
	CNodeDefManager
*/

class CNodeDefManager: public IWritableNodeDefManager
{
public:
	void clear()
	{
		m_content_features.clear();
		m_name_id_mapping.clear();
		m_name_id_mapping_with_aliases.clear();
		m_group_to_items.clear();
		m_next_id = 0;

		u32 initial_length = 0;
		initial_length = MYMAX(initial_length, CONTENT_UNKNOWN + 1);
		initial_length = MYMAX(initial_length, CONTENT_AIR + 1);
		initial_length = MYMAX(initial_length, CONTENT_IGNORE + 1);
		m_content_features.resize(initial_length);

		// Set CONTENT_UNKNOWN
		{
			ContentFeatures f;
			f.name = "unknown";
			// Insert directly into containers
			content_t c = CONTENT_UNKNOWN;
			m_content_features[c] = f;
			addNameIdMapping(c, f.name);
		}

		// Set CONTENT_AIR
		{
			ContentFeatures f;
			f.name                = "air";
			f.drawtype            = NDT_AIRLIKE;
			f.param_type          = CPT_LIGHT;
			f.light_propagates    = true;
			f.sunlight_propagates = true;
			f.walkable            = false;
			f.pointable           = false;
			f.diggable            = false;
			f.buildable_to        = true;
			f.is_ground_content   = true;
			// Insert directly into containers
			content_t c = CONTENT_AIR;
			m_content_features[c] = f;
			addNameIdMapping(c, f.name);
		}

		// Set CONTENT_IGNORE
		{
			ContentFeatures f;
			f.name                = "ignore";
			f.drawtype            = NDT_AIRLIKE;
			f.param_type          = CPT_NONE;
			f.light_propagates    = false;
			f.sunlight_propagates = false;
			f.walkable            = false;
			f.pointable           = false;
			f.diggable            = false;
			f.buildable_to        = true; // A way to remove accidental CONTENT_IGNOREs
			f.is_ground_content   = true;
			// Insert directly into containers
			content_t c = CONTENT_IGNORE;
			m_content_features[c] = f;
			addNameIdMapping(c, f.name);
		}
	}
	CNodeDefManager()
	{
		clear();
	}
	virtual ~CNodeDefManager()
	{
	}
	virtual IWritableNodeDefManager* clone()
	{
		CNodeDefManager *mgr = new CNodeDefManager();
		*mgr = *this;
		return mgr;
	}
	virtual const ContentFeatures& get(content_t c) const
	{
		if(c < m_content_features.size())
			return m_content_features[c];
		else
			return m_content_features[CONTENT_UNKNOWN];
	}
	virtual const ContentFeatures& get(const MapNode &n) const
	{
		return get(n.getContent());
	}
	virtual bool getId(const std::string &name, content_t &result) const
	{
		std::map<std::string, content_t>::const_iterator
			i = m_name_id_mapping_with_aliases.find(name);
		if(i == m_name_id_mapping_with_aliases.end())
			return false;
		result = i->second;
		return true;
	}
	virtual content_t getId(const std::string &name) const
	{
		content_t id = CONTENT_IGNORE;
		getId(name, id);
		return id;
	}
	virtual void getIds(const std::string &name, std::set<content_t> &result)
			const
	{
		//TimeTaker t("getIds", NULL, PRECISION_MICRO);
		if(name.substr(0,6) != "group:"){
			content_t id = CONTENT_IGNORE;
			if(getId(name, id))
				result.insert(id);
			return;
		}
		std::string group = name.substr(6);

		std::map<std::string, GroupItems>::const_iterator
			i = m_group_to_items.find(group);
		if (i == m_group_to_items.end())
			return;

		const GroupItems &items = i->second;
		for (GroupItems::const_iterator j = items.begin();
			j != items.end(); ++j) {
			if ((*j).second != 0)
				result.insert((*j).first);
		}
		//printf("getIds: %dus\n", t.stop());
	}
	virtual const ContentFeatures& get(const std::string &name) const
	{
		content_t id = CONTENT_UNKNOWN;
		getId(name, id);
		return get(id);
	}
	// returns CONTENT_IGNORE if no free ID found
	content_t allocateId()
	{
		for(content_t id = m_next_id;
				id >= m_next_id; // overflow?
				++id){
			while(id >= m_content_features.size()){
				m_content_features.push_back(ContentFeatures());
			}
			const ContentFeatures &f = m_content_features[id];
			if(f.name == ""){
				m_next_id = id + 1;
				return id;
			}
		}
		// If we arrive here, an overflow occurred in id.
		// That means no ID was found
		return CONTENT_IGNORE;
	}
	// IWritableNodeDefManager
	virtual content_t set(const std::string &name,
			const ContentFeatures &def)
	{
		assert(name != "");
		assert(name == def.name);

		// Don't allow redefining ignore (but allow air and unknown)
		if(name == "ignore"){
			infostream<<"NodeDefManager: WARNING: Ignoring "
					<<"CONTENT_IGNORE redefinition"<<std::endl;
			return CONTENT_IGNORE;
		}

		content_t id = CONTENT_IGNORE;
		bool found = m_name_id_mapping.getId(name, id);  // ignore aliases
		if(!found){
			// Get new id
			id = allocateId();
			if(id == CONTENT_IGNORE){
				infostream<<"NodeDefManager: WARNING: Absolute "
						<<"limit reached"<<std::endl;
				return CONTENT_IGNORE;
			}
			assert(id != CONTENT_IGNORE);
			addNameIdMapping(id, name);
		}
		m_content_features[id] = def;
		verbosestream<<"NodeDefManager: registering content id \""<<id
				<<"\": name=\""<<def.name<<"\""<<std::endl;

		// Add this content to the list of all groups it belongs to
		// FIXME: This should remove a node from groups it no longer
		// belongs to when a node is re-registered
		for (ItemGroupList::const_iterator i = def.groups.begin();
			i != def.groups.end(); ++i) {
			std::string group_name = i->first;
			
			std::map<std::string, GroupItems>::iterator
				j = m_group_to_items.find(group_name);
			if (j == m_group_to_items.end()) {
				m_group_to_items[group_name].push_back(
						std::make_pair(id, i->second));
			} else {
				GroupItems &items = j->second;
				items.push_back(std::make_pair(id, i->second));
			}
		}
		return id;
	}
	virtual content_t allocateDummy(const std::string &name)
	{
		assert(name != "");
		ContentFeatures f;
		f.name = name;
		return set(name, f);
	}
	virtual void updateAliases(IItemDefManager *idef)
	{
		std::set<std::string> all = idef->getAll();
		m_name_id_mapping_with_aliases.clear();
		for(std::set<std::string>::iterator
				i = all.begin(); i != all.end(); i++)
		{
			std::string name = *i;
			std::string convert_to = idef->getAlias(name);
			content_t id;
			if(m_name_id_mapping.getId(convert_to, id))
			{
				m_name_id_mapping_with_aliases.insert(
						std::make_pair(name, id));
			}
		}
	}
	virtual void updateTextures(ITextureSource *tsrc)
	{
#ifndef SERVER
		infostream<<"CNodeDefManager::updateTextures(): Updating "
				<<"textures in node definitions"<<std::endl;

		bool new_style_water = g_settings->getBool("new_style_water");
		bool new_style_leaves = g_settings->getBool("new_style_leaves");
		bool opaque_water = g_settings->getBool("opaque_water");

		for(u32 i=0; i<m_content_features.size(); i++)
		{
			ContentFeatures *f = &m_content_features[i];

			// Figure out the actual tiles to use
			TileDef tiledef[6];
			for(u32 j=0; j<6; j++)
			{
				tiledef[j] = f->tiledef[j];
				if(tiledef[j].name == "")
					tiledef[j].name = "unknown_node.png";
			}

			bool is_liquid = false;
			u8 material_type;
			material_type = (f->alpha == 255) ? TILE_MATERIAL_BASIC : TILE_MATERIAL_ALPHA;

			switch(f->drawtype){
			default:
			case NDT_NORMAL:
				f->solidness = 2;
				break;
			case NDT_AIRLIKE:
				f->solidness = 0;
				break;
			case NDT_LIQUID:
				assert(f->liquid_type == LIQUID_SOURCE);
				if(opaque_water)
					f->alpha = 255;
				if(new_style_water){
					f->solidness = 0;
				} else {
					f->solidness = 1;
					f->backface_culling = false;
				}
				is_liquid = true;
				break;
			case NDT_FLOWINGLIQUID:
				assert(f->liquid_type == LIQUID_FLOWING);
				f->solidness = 0;
				if(opaque_water)
					f->alpha = 255;
				is_liquid = true;
				break;
			case NDT_GLASSLIKE:
				f->solidness = 0;
				f->visual_solidness = 1;
				break;
			case NDT_GLASSLIKE_FRAMED:
				f->solidness = 0;
				f->visual_solidness = 1;
				break;
			case NDT_ALLFACES:
				f->solidness = 0;
				f->visual_solidness = 1;
				break;
			case NDT_ALLFACES_OPTIONAL:
				if(new_style_leaves){
					f->drawtype = NDT_ALLFACES;
					f->solidness = 0;
					f->visual_solidness = 1;
				} else {
					f->drawtype = NDT_NORMAL;
					f->solidness = 2;
					for(u32 i=0; i<6; i++){
						tiledef[i].name += std::string("^[noalpha");
					}
				}
				if (f->waving == 1)
					material_type = TILE_MATERIAL_LEAVES;
				break;
			case NDT_PLANTLIKE:
				f->solidness = 0;
				f->backface_culling = false;
				if (f->waving == 1)
					material_type = TILE_MATERIAL_PLANTS;
				break;
			case NDT_TORCHLIKE:
			case NDT_SIGNLIKE:
			case NDT_FENCELIKE:
			case NDT_RAILLIKE:
			case NDT_NODEBOX:
				f->solidness = 0;
				break;
			}

			if (is_liquid)
				material_type = (f->alpha == 255) ? TILE_MATERIAL_LIQUID_OPAQUE : TILE_MATERIAL_LIQUID_TRANSPARENT;

			// Tiles (fill in f->tiles[])
			for(u16 j=0; j<6; j++){
				// Texture
				f->tiles[j].texture = tsrc->getTexture(
						tiledef[j].name,
						&f->tiles[j].texture_id);
				// Alpha
				f->tiles[j].alpha = f->alpha;
				// Material type
				f->tiles[j].material_type = material_type;
				// Material flags
				f->tiles[j].material_flags = 0;
				if(f->backface_culling)
					f->tiles[j].material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
				if(tiledef[j].animation.type == TAT_VERTICAL_FRAMES)
					f->tiles[j].material_flags |= MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;
				// Animation parameters
				if(f->tiles[j].material_flags &
						MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES)
				{
					// Get texture size to determine frame count by
					// aspect ratio
					v2u32 size = f->tiles[j].texture->getOriginalSize();
					int frame_height = (float)size.X /
							(float)tiledef[j].animation.aspect_w *
							(float)tiledef[j].animation.aspect_h;
					int frame_count = size.Y / frame_height;
					int frame_length_ms = 1000.0 *
							tiledef[j].animation.length / frame_count;
					f->tiles[j].animation_frame_count = frame_count;
					f->tiles[j].animation_frame_length_ms = frame_length_ms;

					// If there are no frames for an animation, switch
					// animation off (so that having specified an animation
					// for something but not using it in the texture pack
					// gives no overhead)
					if(frame_count == 1){
						f->tiles[j].material_flags &=
								~MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;
					}
				}
			}
			// Special tiles (fill in f->special_tiles[])
			for(u16 j=0; j<CF_SPECIAL_COUNT; j++){
				// Texture
				f->special_tiles[j].texture = tsrc->getTexture(
						f->tiledef_special[j].name,
						&f->special_tiles[j].texture_id);
				// Alpha
				f->special_tiles[j].alpha = f->alpha;
				// Material type
				f->special_tiles[j].material_type = material_type;
				// Material flags
				f->special_tiles[j].material_flags = 0;
				if(f->tiledef_special[j].backface_culling)
					f->special_tiles[j].material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
				if(f->tiledef_special[j].animation.type == TAT_VERTICAL_FRAMES)
					f->special_tiles[j].material_flags |= MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;
				// Animation parameters
				if(f->special_tiles[j].material_flags &
						MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES)
				{
					// Get texture size to determine frame count by
					// aspect ratio
					v2u32 size = f->special_tiles[j].texture->getOriginalSize();
					int frame_height = (float)size.X /
							(float)f->tiledef_special[j].animation.aspect_w *
							(float)f->tiledef_special[j].animation.aspect_h;
					int frame_count = size.Y / frame_height;
					int frame_length_ms = 1000.0 *
							f->tiledef_special[j].animation.length / frame_count;
					f->special_tiles[j].animation_frame_count = frame_count;
					f->special_tiles[j].animation_frame_length_ms = frame_length_ms;

					// If there are no frames for an animation, switch
					// animation off (so that having specified an animation
					// for something but not using it in the texture pack
					// gives no overhead)
					if(frame_count == 1){
						f->special_tiles[j].material_flags &=
								~MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES;
					}
				}
			}
		}
#endif
	}
	void serialize(std::ostream &os, u16 protocol_version)
	{
		writeU8(os, 1); // version
		u16 count = 0;
		std::ostringstream os2(std::ios::binary);
		for(u32 i=0; i<m_content_features.size(); i++)
		{
			if(i == CONTENT_IGNORE || i == CONTENT_AIR
					|| i == CONTENT_UNKNOWN)
				continue;
			ContentFeatures *f = &m_content_features[i];
			if(f->name == "")
				continue;
			writeU16(os2, i);
			// Wrap it in a string to allow different lengths without
			// strict version incompatibilities
			std::ostringstream wrapper_os(std::ios::binary);
			f->serialize(wrapper_os, protocol_version);
			os2<<serializeString(wrapper_os.str());

			assert(count + 1 > count); // must not overflow
			count++;
		}
		writeU16(os, count);
		os<<serializeLongString(os2.str());
	}
	void deSerialize(std::istream &is)
	{
		clear();
		int version = readU8(is);
		if(version != 1)
			throw SerializationError("unsupported NodeDefinitionManager version");
		u16 count = readU16(is);
		std::istringstream is2(deSerializeLongString(is), std::ios::binary);
		ContentFeatures f;
		for(u16 n=0; n<count; n++){
			u16 i = readU16(is2);

			// Read it from the string wrapper
			std::string wrapper = deSerializeString(is2);
			std::istringstream wrapper_is(wrapper, std::ios::binary);
			f.deSerialize(wrapper_is);

			// Check error conditions
			if(i == CONTENT_IGNORE || i == CONTENT_AIR
					|| i == CONTENT_UNKNOWN){
				infostream<<"NodeDefManager::deSerialize(): WARNING: "
					<<"not changing builtin node "<<i
					<<std::endl;
				continue;
			}
			if(f.name == ""){
				infostream<<"NodeDefManager::deSerialize(): WARNING: "
					<<"received empty name"<<std::endl;
				continue;
			}
			u16 existing_id;
			bool found = m_name_id_mapping.getId(f.name, existing_id);  // ignore aliases
			if(found && i != existing_id){
				infostream<<"NodeDefManager::deSerialize(): WARNING: "
					<<"already defined with different ID: "
					<<f.name<<std::endl;
				continue;
			}

			// All is ok, add node definition with the requested ID
			if(i >= m_content_features.size())
				m_content_features.resize((u32)(i) + 1);
			m_content_features[i] = f;
			addNameIdMapping(i, f.name);
			verbosestream<<"deserialized "<<f.name<<std::endl;
		}
	}
private:
	void addNameIdMapping(content_t i, std::string name)
	{
		m_name_id_mapping.set(i, name);
		m_name_id_mapping_with_aliases.insert(std::make_pair(name, i));
	}
private:
	// Features indexed by id
	std::vector<ContentFeatures> m_content_features;
	// A mapping for fast converting back and forth between names and ids
	NameIdMapping m_name_id_mapping;
	// Like m_name_id_mapping, but only from names to ids, and includes
	// item aliases too. Updated by updateAliases()
	// Note: Not serialized.
	std::map<std::string, content_t> m_name_id_mapping_with_aliases;
	// A mapping from groups to a list of content_ts (and their levels)
	// that belong to it.  Necessary for a direct lookup in getIds().
	// Note: Not serialized.
	std::map<std::string, GroupItems> m_group_to_items;
	// Next possibly free id
	content_t m_next_id;
};

IWritableNodeDefManager* createNodeDefManager()
{
	return new CNodeDefManager();
}

/*
	Serialization of old ContentFeatures formats
*/

void ContentFeatures::serializeOld(std::ostream &os, u16 protocol_version) const
{
	if(protocol_version == 14)
	{
		writeU8(os, 6); // version
		os<<serializeString(name);
		writeU16(os, groups.size());
		for(ItemGroupList::const_iterator
				i = groups.begin(); i != groups.end(); i++){
			os<<serializeString(i->first);
			writeS16(os, i->second);
		}
		writeU8(os, drawtype);
		writeF1000(os, visual_scale);
		writeU8(os, 6);
		for(u32 i=0; i<6; i++)
			tiledef[i].serialize(os, protocol_version);
		writeU8(os, CF_SPECIAL_COUNT);
		for(u32 i=0; i<CF_SPECIAL_COUNT; i++){
			tiledef_special[i].serialize(os, protocol_version);
		}
		writeU8(os, alpha);
		writeU8(os, post_effect_color.getAlpha());
		writeU8(os, post_effect_color.getRed());
		writeU8(os, post_effect_color.getGreen());
		writeU8(os, post_effect_color.getBlue());
		writeU8(os, param_type);
		writeU8(os, param_type_2);
		writeU8(os, is_ground_content);
		writeU8(os, light_propagates);
		writeU8(os, sunlight_propagates);
		writeU8(os, walkable);
		writeU8(os, pointable);
		writeU8(os, diggable);
		writeU8(os, climbable);
		writeU8(os, buildable_to);
		os<<serializeString(""); // legacy: used to be metadata_name
		writeU8(os, liquid_type);
		os<<serializeString(liquid_alternative_flowing);
		os<<serializeString(liquid_alternative_source);
		writeU8(os, liquid_viscosity);
		writeU8(os, liquid_renewable);
		writeU8(os, light_source);
		writeU32(os, damage_per_second);
		node_box.serialize(os, protocol_version);
		selection_box.serialize(os, protocol_version);
		writeU8(os, legacy_facedir_simple);
		writeU8(os, legacy_wallmounted);
		serializeSimpleSoundSpec(sound_footstep, os);
		serializeSimpleSoundSpec(sound_dig, os);
		serializeSimpleSoundSpec(sound_dug, os);
	}
	else if(protocol_version == 13)
	{
		writeU8(os, 5); // version
		os<<serializeString(name);
		writeU16(os, groups.size());
		for(ItemGroupList::const_iterator
				i = groups.begin(); i != groups.end(); i++){
			os<<serializeString(i->first);
			writeS16(os, i->second);
		}
		writeU8(os, drawtype);
		writeF1000(os, visual_scale);
		writeU8(os, 6);
		for(u32 i=0; i<6; i++)
			tiledef[i].serialize(os, protocol_version);
		writeU8(os, CF_SPECIAL_COUNT);
		for(u32 i=0; i<CF_SPECIAL_COUNT; i++){
			tiledef_special[i].serialize(os, protocol_version);
		}
		writeU8(os, alpha);
		writeU8(os, post_effect_color.getAlpha());
		writeU8(os, post_effect_color.getRed());
		writeU8(os, post_effect_color.getGreen());
		writeU8(os, post_effect_color.getBlue());
		writeU8(os, param_type);
		writeU8(os, param_type_2);
		writeU8(os, is_ground_content);
		writeU8(os, light_propagates);
		writeU8(os, sunlight_propagates);
		writeU8(os, walkable);
		writeU8(os, pointable);
		writeU8(os, diggable);
		writeU8(os, climbable);
		writeU8(os, buildable_to);
		os<<serializeString(""); // legacy: used to be metadata_name
		writeU8(os, liquid_type);
		os<<serializeString(liquid_alternative_flowing);
		os<<serializeString(liquid_alternative_source);
		writeU8(os, liquid_viscosity);
		writeU8(os, light_source);
		writeU32(os, damage_per_second);
		node_box.serialize(os, protocol_version);
		selection_box.serialize(os, protocol_version);
		writeU8(os, legacy_facedir_simple);
		writeU8(os, legacy_wallmounted);
		serializeSimpleSoundSpec(sound_footstep, os);
		serializeSimpleSoundSpec(sound_dig, os);
		serializeSimpleSoundSpec(sound_dug, os);
	}
	else
	{
		throw SerializationError("ContentFeatures::serialize(): Unsupported version requested");
	}
}

void ContentFeatures::deSerializeOld(std::istream &is, int version)
{
	if(version == 6) // In PROTOCOL_VERSION 14
	{
		name = deSerializeString(is);
		groups.clear();
		u32 groups_size = readU16(is);
		for(u32 i=0; i<groups_size; i++){
			std::string name = deSerializeString(is);
			int value = readS16(is);
			groups[name] = value;
		}
		drawtype = (enum NodeDrawType)readU8(is);
		visual_scale = readF1000(is);
		if(readU8(is) != 6)
			throw SerializationError("unsupported tile count");
		for(u32 i=0; i<6; i++)
			tiledef[i].deSerialize(is);
		if(readU8(is) != CF_SPECIAL_COUNT)
			throw SerializationError("unsupported CF_SPECIAL_COUNT");
		for(u32 i=0; i<CF_SPECIAL_COUNT; i++)
			tiledef_special[i].deSerialize(is);
		alpha = readU8(is);
		post_effect_color.setAlpha(readU8(is));
		post_effect_color.setRed(readU8(is));
		post_effect_color.setGreen(readU8(is));
		post_effect_color.setBlue(readU8(is));
		param_type = (enum ContentParamType)readU8(is);
		param_type_2 = (enum ContentParamType2)readU8(is);
		is_ground_content = readU8(is);
		light_propagates = readU8(is);
		sunlight_propagates = readU8(is);
		walkable = readU8(is);
		pointable = readU8(is);
		diggable = readU8(is);
		climbable = readU8(is);
		buildable_to = readU8(is);
		deSerializeString(is); // legacy: used to be metadata_name
		liquid_type = (enum LiquidType)readU8(is);
		liquid_alternative_flowing = deSerializeString(is);
		liquid_alternative_source = deSerializeString(is);
		liquid_viscosity = readU8(is);
		liquid_renewable = readU8(is);
		light_source = readU8(is);
		damage_per_second = readU32(is);
		node_box.deSerialize(is);
		selection_box.deSerialize(is);
		legacy_facedir_simple = readU8(is);
		legacy_wallmounted = readU8(is);
		deSerializeSimpleSoundSpec(sound_footstep, is);
		deSerializeSimpleSoundSpec(sound_dig, is);
		deSerializeSimpleSoundSpec(sound_dug, is);
		// If you add anything here, insert it primarily inside the try-catch
		// block to not need to increase the version.
		try{
			// Stuff below should be moved to correct place in a version that
			// otherwise changes the protocol version
		}catch(SerializationError &e) {};
	}
	else if(version == 5) // In PROTOCOL_VERSION 13
	{
		name = deSerializeString(is);
		groups.clear();
		u32 groups_size = readU16(is);
		for(u32 i=0; i<groups_size; i++){
			std::string name = deSerializeString(is);
			int value = readS16(is);
			groups[name] = value;
		}
		drawtype = (enum NodeDrawType)readU8(is);
		visual_scale = readF1000(is);
		if(readU8(is) != 6)
			throw SerializationError("unsupported tile count");
		for(u32 i=0; i<6; i++)
			tiledef[i].deSerialize(is);
		if(readU8(is) != CF_SPECIAL_COUNT)
			throw SerializationError("unsupported CF_SPECIAL_COUNT");
		for(u32 i=0; i<CF_SPECIAL_COUNT; i++)
			tiledef_special[i].deSerialize(is);
		alpha = readU8(is);
		post_effect_color.setAlpha(readU8(is));
		post_effect_color.setRed(readU8(is));
		post_effect_color.setGreen(readU8(is));
		post_effect_color.setBlue(readU8(is));
		param_type = (enum ContentParamType)readU8(is);
		param_type_2 = (enum ContentParamType2)readU8(is);
		is_ground_content = readU8(is);
		light_propagates = readU8(is);
		sunlight_propagates = readU8(is);
		walkable = readU8(is);
		pointable = readU8(is);
		diggable = readU8(is);
		climbable = readU8(is);
		buildable_to = readU8(is);
		deSerializeString(is); // legacy: used to be metadata_name
		liquid_type = (enum LiquidType)readU8(is);
		liquid_alternative_flowing = deSerializeString(is);
		liquid_alternative_source = deSerializeString(is);
		liquid_viscosity = readU8(is);
		light_source = readU8(is);
		damage_per_second = readU32(is);
		node_box.deSerialize(is);
		selection_box.deSerialize(is);
		legacy_facedir_simple = readU8(is);
		legacy_wallmounted = readU8(is);
		deSerializeSimpleSoundSpec(sound_footstep, is);
		deSerializeSimpleSoundSpec(sound_dig, is);
		deSerializeSimpleSoundSpec(sound_dug, is);
	}
	else
	{
		throw SerializationError("unsupported ContentFeatures version");
	}
}

