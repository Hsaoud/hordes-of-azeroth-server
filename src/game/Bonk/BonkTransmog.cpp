#include "BonkTransmog.h"
#include "BonkMgr.h"
#include "BonkPlayer.h"
#include "BonkStrings.h"
#include "Chat.h"
#include "Database/DatabaseEnv.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Policies/SingletonImp.h"
#include "SharedDefines.h"
#include "WorldSession.h"

#include <cstdio>

INSTANTIATE_SINGLETON_1(BonkTransmogStore);

namespace
{
    enum BonkLookGroup : uint8
    {
        BONK_LOOK_NONE = 0,
        BONK_LOOK_HEAD,
        BONK_LOOK_SHOULDERS,
        BONK_LOOK_SHIRT,
        BONK_LOOK_CHEST,
        BONK_LOOK_WAIST,
        BONK_LOOK_LEGS,
        BONK_LOOK_FEET,
        BONK_LOOK_WRISTS,
        BONK_LOOK_HANDS,
        BONK_LOOK_BACK,
        BONK_LOOK_TABARD,
        BONK_LOOK_DAGGER,
        BONK_LOOK_ONE_HAND,
        BONK_LOOK_TWO_HAND,
        BONK_LOOK_RANGED,
        BONK_LOOK_SHIELD,
        BONK_LOOK_HELD,
    };

    uint8 const BONK_TRANSMOG_WINDOW_SLOTS[] = { EQUIPMENT_SLOT_HEAD, EQUIPMENT_SLOT_SHOULDERS, EQUIPMENT_SLOT_BACK, EQUIPMENT_SLOT_CHEST,
        EQUIPMENT_SLOT_BODY, EQUIPMENT_SLOT_TABARD, EQUIPMENT_SLOT_WRISTS, EQUIPMENT_SLOT_HANDS, EQUIPMENT_SLOT_WAIST, EQUIPMENT_SLOT_LEGS,
        EQUIPMENT_SLOT_FEET, EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND, EQUIPMENT_SLOT_RANGED };

    uint8 GetArmorGroup(uint32 inventoryType)
    {
        switch (inventoryType)
        {
            case INVTYPE_HEAD:      return BONK_LOOK_HEAD;
            case INVTYPE_SHOULDERS: return BONK_LOOK_SHOULDERS;
            case INVTYPE_BODY:      return BONK_LOOK_SHIRT;
            case INVTYPE_CHEST:
            case INVTYPE_ROBE:      return BONK_LOOK_CHEST;
            case INVTYPE_WAIST:     return BONK_LOOK_WAIST;
            case INVTYPE_LEGS:      return BONK_LOOK_LEGS;
            case INVTYPE_FEET:      return BONK_LOOK_FEET;
            case INVTYPE_WRISTS:    return BONK_LOOK_WRISTS;
            case INVTYPE_HANDS:     return BONK_LOOK_HANDS;
            case INVTYPE_CLOAK:     return BONK_LOOK_BACK;
            case INVTYPE_TABARD:    return BONK_LOOK_TABARD;
            case INVTYPE_HOLDABLE:  return BONK_LOOK_HELD;
            default:                return BONK_LOOK_NONE;
        }
    }

    uint8 GetWeaponGroup(uint32 subClass)
    {
        switch (subClass)
        {
            case ITEM_SUBCLASS_WEAPON_DAGGER:   return BONK_LOOK_DAGGER;
            case ITEM_SUBCLASS_WEAPON_AXE:
            case ITEM_SUBCLASS_WEAPON_MACE:
            case ITEM_SUBCLASS_WEAPON_SWORD:
            case ITEM_SUBCLASS_WEAPON_FIST:     return BONK_LOOK_ONE_HAND;
            case ITEM_SUBCLASS_WEAPON_AXE2:
            case ITEM_SUBCLASS_WEAPON_MACE2:
            case ITEM_SUBCLASS_WEAPON_POLEARM:
            case ITEM_SUBCLASS_WEAPON_SWORD2:
            case ITEM_SUBCLASS_WEAPON_STAFF:    return BONK_LOOK_TWO_HAND;
            case ITEM_SUBCLASS_WEAPON_BOW:
            case ITEM_SUBCLASS_WEAPON_GUN:
            case ITEM_SUBCLASS_WEAPON_THROWN:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
            case ITEM_SUBCLASS_WEAPON_WAND:     return BONK_LOOK_RANGED;
            default:                            return BONK_LOOK_NONE;
        }
    }

    uint32 GetAccount(Player const* player)
    {
        return player->GetSession()->GetAccountId();
    }

    std::string ItemLink(ItemPrototype const* proto)
    {
        char link[256];
        snprintf(link, sizeof(link), "|c%08x|Hitem:%u:0:0:0|h[%s]|h|r", ItemQualityColors[proto->Quality < MAX_ITEM_QUALITY ? proto->Quality : 0],
            proto->ItemId, proto->Name1);
        return link;
    }
}

uint8 BonkTransmog::GetLookGroup(ItemPrototype const* proto)
{
    if (!proto)
        return BONK_LOOK_NONE;
    if (proto->Class == ITEM_CLASS_WEAPON)
        return GetWeaponGroup(proto->SubClass);
    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
        return BONK_LOOK_SHIELD;
    if (proto->Class == ITEM_CLASS_ARMOR)
        return GetArmorGroup(proto->InventoryType);
    return BONK_LOOK_NONE;
}

bool BonkTransmog::IsTransmogSlot(uint8 slot)
{
    for (uint8 each : BONK_TRANSMOG_WINDOW_SLOTS)
        if (each == slot)
            return true;
    return false;
}

bool BonkTransmog::Fits(ItemPrototype const* look, ItemPrototype const* worn)
{
    uint8 const group = GetLookGroup(worn);
    return group != BONK_LOOK_NONE && group == GetLookGroup(look);
}

void BonkTransmogStore::LoadAccount(uint32 accountId)
{
    std::unique_ptr<QueryResult> result(CharacterDatabase.PQuery("SELECT `entry` FROM `bonk_account_appearance` WHERE `account` = %u", accountId));
    std::lock_guard<std::mutex> guard(m_lock);
    std::set<uint32>& collection = m_collections[accountId];
    if (result)
    {
        do
            collection.insert(result->Fetch()[0].GetUInt32());
        while (result->NextRow());
    }
}

void BonkTransmogStore::LoadCharacter(uint32 characterGuid)
{
    std::unique_ptr<QueryResult> result(CharacterDatabase.PQuery("SELECT `slot`, `entry` FROM `bonk_character_appearance` WHERE `guid` = %u", characterGuid));
    std::lock_guard<std::mutex> guard(m_lock);
    std::array<uint32, BONK_TRANSMOG_SLOTS>& choices = m_choices[characterGuid];
    choices.fill(0);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint8 const slot = fields[0].GetUInt8();
            if (slot < BONK_TRANSMOG_SLOTS)
                choices[slot] = fields[1].GetUInt32();
        } while (result->NextRow());
    }
}

void BonkTransmogStore::ForgetCharacter(uint32 characterGuid)
{
    std::lock_guard<std::mutex> guard(m_lock);
    m_choices.erase(characterGuid);
}

void BonkTransmogStore::Collect(Player* player, uint32 entry, bool announce)
{
    ItemPrototype const* proto = sObjectMgr.GetItemPrototype(entry);
    if (!player || BonkTransmog::GetLookGroup(proto) == BONK_LOOK_NONE)
        return;
    uint32 const accountId = GetAccount(player);
    {
        std::lock_guard<std::mutex> guard(m_lock);
        if (!m_collections[accountId].insert(entry).second)
            return;
    }
    CharacterDatabase.PExecute("INSERT IGNORE INTO `bonk_account_appearance` (`account`, `entry`) VALUES (%u, %u)", accountId, entry);
    if (announce && player->IsInWorld())
        if (BonkPlayer* bonk = player->GetBonkPlayer())
            bonk->Notify(BONK_STRING_APPEARANCE_COLLECTED, { ItemLink(proto) });
}

void BonkTransmogStore::CollectWorn(Player* player)
{
    for (uint8 slot = 0; slot < BONK_TRANSMOG_SLOTS; ++slot)
    {
        if (!BonkTransmog::IsTransmogSlot(slot))
            continue;
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            Collect(player, item->GetEntry(), false);
        RefreshSlot(player, slot);
    }
}

bool BonkTransmogStore::IsCollected(uint32 accountId, uint32 entry) const
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto collection = m_collections.find(accountId);
    return collection != m_collections.end() && collection->second.count(entry);
}

uint32 BonkTransmogStore::GetChoice(uint32 characterGuid, uint8 slot) const
{
    std::lock_guard<std::mutex> guard(m_lock);
    auto choices = m_choices.find(characterGuid);
    return choices != m_choices.end() && slot < BONK_TRANSMOG_SLOTS ? choices->second[slot] : 0;
}

void BonkTransmogStore::SetChoice(uint32 characterGuid, uint8 slot, uint32 entry)
{
    {
        std::lock_guard<std::mutex> guard(m_lock);
        std::array<uint32, BONK_TRANSMOG_SLOTS>& choices = m_choices[characterGuid];
        choices[slot] = entry;
    }
    if (entry)
        CharacterDatabase.PExecute("REPLACE INTO `bonk_character_appearance` (`guid`, `slot`, `entry`) VALUES (%u, %u, %u)", characterGuid, slot, entry);
    else
        CharacterDatabase.PExecute("DELETE FROM `bonk_character_appearance` WHERE `guid` = %u AND `slot` = %u", characterGuid, slot);
}

uint32 BonkTransmogStore::GetVisibleEntry(Player const* player, uint8 slot, Item const* item) const
{
    if (!item)
        return 0;
    uint32 const choice = GetChoice(player->GetGUIDLow(), slot);
    if (!choice || !IsCollected(GetAccount(player), choice))
        return item->GetEntry();
    return BonkTransmog::Fits(sObjectMgr.GetItemPrototype(choice), item->GetProto()) ? choice : item->GetEntry();
}

void BonkTransmogStore::RefreshSlot(Player* player, uint8 slot)
{
    player->SetVisibleItemSlot(slot, player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
}

bool BonkTransmogStore::Apply(Player* player, uint8 slot, uint32 entry, std::string& error)
{
    BonkPlayer* bonk = player->GetBonkPlayer();
    Item const* worn = BonkTransmog::IsTransmogSlot(slot) ? player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot) : nullptr;
    if (!worn)
    {
        error = bonk->GetString(BONK_STRING_TRANSMOG_EQUIP_FIRST);
        return false;
    }
    if (!IsCollected(GetAccount(player), entry))
    {
        error = bonk->GetString(BONK_STRING_TRANSMOG_NOT_COLLECTED);
        return false;
    }
    if (!BonkTransmog::Fits(sObjectMgr.GetItemPrototype(entry), worn->GetProto()))
    {
        error = bonk->GetString(BONK_STRING_TRANSMOG_DOESNT_FIT);
        return false;
    }
    SetChoice(player->GetGUIDLow(), slot, entry == worn->GetEntry() ? 0 : entry);
    RefreshSlot(player, slot);
    SendWindow(player);
    return true;
}

void BonkTransmogStore::Clear(Player* player, uint8 slot)
{
    if (slot >= BONK_TRANSMOG_SLOTS)
        return;
    SetChoice(player->GetGUIDLow(), slot, 0);
    RefreshSlot(player, slot);
    SendWindow(player);
}

void BonkTransmogStore::SendWindow(Player* player) const
{
    BonkPlayer* bonk = player->GetBonkPlayer();
    if (!bonk || !bonk->IsAddonReady())
        return;
    bonk->SendAddon("mogbegin");
    for (uint8 slot : BONK_TRANSMOG_WINDOW_SLOTS)
    {
        Item const* worn = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        uint32 const wornEntry = worn ? worn->GetEntry() : 0;
        uint32 const shown = GetVisibleEntry(player, slot, worn);
        uint8 const group = worn ? BonkTransmog::GetLookGroup(worn->GetProto()) : BONK_LOOK_NONE;
        bonk->SendAddon("mogslot~" + std::to_string(slot) + "~" + std::to_string(wornEntry) + "~" + std::to_string(shown != wornEntry ? shown : 0) +
            "~" + std::to_string(group));
    }
    std::set<uint32> collection;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        auto found = m_collections.find(GetAccount(player));
        if (found != m_collections.end())
            collection = found->second;
    }
    std::set<std::pair<uint8, uint32>> shownLooks;
    for (uint32 entry : collection)
    {
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(entry);
        uint8 const group = BonkTransmog::GetLookGroup(proto);
        if (group == BONK_LOOK_NONE || !shownLooks.insert({ group, proto->DisplayInfoID }).second)
            continue;
        bonk->SendAddon("moglook~" + std::to_string(group) + "~" + std::to_string(entry) + "~" + std::to_string(proto->Quality));
    }
    bonk->SendAddon("mogend");
}

bool ChatHandler::HandleBonkMogCommand(char* args)
{
    Player* player = m_session->GetPlayer();
    BonkPlayer* bonk = player->GetBonkPlayer();
    std::string const text = args ? args : "";
    std::string error;
    if (bonk->GetRun())
        error = bonk->GetString(BONK_STRING_TRANSMOG_ONLY_IN_TOWN);
    else if (text == "open")
        sBonkTransmog.SendWindow(player);
    else if (text.compare(0, 6, "clear ") == 0)
        sBonkTransmog.Clear(player, uint8(atoi(text.c_str() + 6)));
    else if (text.compare(0, 4, "set ") == 0)
    {
        unsigned int slot = 0, entry = 0;
        if (sscanf(text.c_str() + 4, "%u %u", &slot, &entry) == 2)
            sBonkTransmog.Apply(player, uint8(slot), entry, error);
    }
    if (!error.empty())
        bonk->Notify(error);
    return true;
}
