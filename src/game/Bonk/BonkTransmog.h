#ifndef BONK_TRANSMOG_H
#define BONK_TRANSMOG_H

#include "Common.h"
#include "Policies/Singleton.h"

#include <array>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Item;
class Player;
struct ItemPrototype;

uint32 const BONK_TRANSMOGRIFIER_ENTRY = 900170;
uint32 const BONK_BLANK_TABARD_ENTRY = 939000;
uint8 const BONK_TRANSMOG_SLOTS = 19;

class BonkTransmogStore
{
    public:
        void LoadAccount(uint32 accountId);
        void LoadCharacter(uint32 characterGuid);
        void ForgetCharacter(uint32 characterGuid);

        void Collect(Player* player, uint32 entry, bool announce = true);
        void CollectWorn(Player* player);
        bool IsCollected(uint32 accountId, uint32 entry) const;
        uint32 GetVisibleEntry(Player const* player, uint8 slot, Item const* item) const;

        bool Apply(Player* player, uint8 slot, uint32 entry, std::string& error);
        void Clear(Player* player, uint8 slot);
        void SendWindow(Player* player) const;

    private:
        mutable std::mutex m_lock;
        std::unordered_map<uint32, std::set<uint32>> m_collections;
        std::unordered_map<uint32, std::set<uint32>> m_looks;
        std::unordered_map<uint32, std::array<uint32, BONK_TRANSMOG_SLOTS>> m_choices;

        uint32 GetChoice(uint32 characterGuid, uint8 slot) const;
        void SetChoice(uint32 characterGuid, uint8 slot, uint32 entry);
        void RefreshSlot(Player* player, uint8 slot);
};

#define sBonkTransmog MaNGOS::Singleton<BonkTransmogStore>::Instance()

namespace BonkTransmog
{
    uint8 GetLookGroup(ItemPrototype const* proto);
    bool IsTransmogSlot(uint8 slot);
    bool Fits(ItemPrototype const* look, ItemPrototype const* worn);
}

#endif
