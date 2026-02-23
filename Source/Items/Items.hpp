#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct ItemDefinition {
	std::uint32_t Id = 0;

	std::uint8_t Editable_Type = 0;
	std::uint8_t Item_Category = 0;
	std::uint8_t Material_Type = 0;
	std::uint8_t Hit_Sound_Type = 0;

	std::string Name = "";
	std::string Texture = "";
	std::uint32_t Texture_Hash = 0;

	std::uint8_t Visual_Effect_Type = 0;
	std::uint32_t Cooking_Time = 0;

	std::uint8_t Texture_X = 0;
	std::uint8_t Texture_Y = 0;

	std::uint8_t Storage_Type = 0;
	std::uint8_t Is_Stripey_Wallpaper = 0;
	std::uint8_t Collision_Type = 0;

	std::uint8_t Break_Hits = 0;

	std::uint32_t Reset_State_After = 0;
	std::uint32_t Clothing_Type = 0;
	std::uint32_t Rarity = 0;
	std::uint8_t Max_Amount = 0;

	std::string Extra_File = "";
	std::uint32_t Extra_File_Hash = 0;

	std::uint32_t Audio_Volume = 0;

	std::string Pet_Name = "";
	std::string Pet_Prefix = "";
	std::string Pet_Suffix = "";
	std::string Pet_Ability = "";

	std::uint8_t Seed_Base = 0;
	std::uint8_t Seed_Overlay = 0;
	std::uint8_t Tree_Base = 0;
	std::uint8_t Tree_Leaves = 0;

	std::uint32_t Seed_Color = 0;
	std::uint32_t Seed_Overlay_Color = 0;
	std::uint32_t Ingredient = 0;
	std::uint32_t Grow_Time = 0;

	std::uint16_t Flags2 = 0;
	std::uint16_t Rayman = 0;

	std::string Extra_Options = "";
	std::string Texture2_Path = "";
	std::string Extra_Options2 = "";

	std::uint8_t Unk_Data1[80] = { 0 };

	// version 11
	std::string Punch_Options = "";

	// version 12
	std::uint32_t Flags3 = 0;
	std::uint8_t Bodypart[9] = { 0 };

	// version 13
	std::uint32_t Flags4 = 0;

	// version 14
	std::uint32_t Flags5 = 0;

	// version 15
	std::uint8_t Unk_Data2[25] = { 0 };
	std::string Texture3_Path = "";

	// version 23
	uint16_t Ingredient1;
	uint16_t Ingredient2;
};

class gItems {
public:
	std::string Inject();

	const ItemDefinition* GetItemByID(int32_t id) const;
	ItemDefinition* GetItemByID(int32_t id);

	const ItemDefinition* GetItemByName(const std::string& name) const;
	ItemDefinition* GetItemByName(const std::string& name);

private:
	std::vector<uint8_t> packetData;

	std::vector<ItemDefinition> items;
	std::unordered_map<int32_t, size_t> itemID;
	std::unordered_map<std::string, size_t> itemNAME;
};

extern gItems Items;