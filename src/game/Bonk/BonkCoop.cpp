#include "BonkPlayer.h"
#include "BonkRun.h"
#include "BonkMobAI.h"
#include "Config/Config.h"
#include "Creature.h"
#include "Group.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"

Player* BonkPlayer::GetCoopFriend() const
{
    if (!sConfig.GetBoolDefault("Bonk.Coop", false) || !m_player->IsInWorld())
        return nullptr;
    for (auto const& ref : m_player->GetMap()->GetPlayers())
    {
        Player* other = ref.getSource();
        if (other && other != m_player && other->IsInWorld() && !other->IsBeingTeleported())
            return other;
    }
    return nullptr;
}

void BonkPlayer::UpdateCoop(uint32 diff)
{
    if (!sConfig.GetBoolDefault("Bonk.Coop", false))
        return;
    if (m_coopTier)
    {
        m_coopReadyMs += diff;
        if (m_coopReadyMs >= 120000 || !IsAtCamp() || m_player->IsInCombat() || !GetCoopFriend())
            m_coopTier = 0;
    }
    m_coopTimer += diff;
    if (m_coopTimer < 1000)
        return;
    m_coopTimer = 0;
    if (!m_run && IsAtCamp())
        if (Player* other = GetCoopFriend())
            if (!other->GetBonkPlayer()->GetRun() && other->GetBonkPlayer()->IsAtCamp())
            {
                Group* group = m_player->GetGroup();
                if (!group && !other->GetGroup())
                {
                    group = new Group;
                    if (group->Create(m_player->GetObjectGuid(), m_player->GetName()))
                        sObjectMgr.AddGroup(group);
                    else
                    {
                        delete group;
                        group = nullptr;
                    }
                }
                if (group && !other->GetGroup())
                {
                    group->AddMember(other->GetObjectGuid(), other->GetName());
                    group->BroadcastGroupUpdate();
                }
                if (group && other->GetGroup() == group)
                    other->SetFactionTemplateId(m_player->GetFactionTemplateId());
            }
    SendCoopStatus();
}

void BonkPlayer::SendCoopStatus()
{
    if (!IsAddonReady())
        return;
    Player* other = GetCoopFriend();
    BonkPlayer* friendBonk = other ? other->GetBonkPlayer() : nullptr;
    uint32 const proposal = m_coopTier ? m_coopTier : friendBonk ? friendBonk->m_coopTier : 0;
    BonkTier const* tier = proposal ? sBonkMgr.GetTier(proposal) : nullptr;
    std::string line = "coop~" + std::string(other ? other->GetName() : "") + "~" + (m_coopTier ? "1" : "0") + "~" +
        (tier ? tier->name : "") + "~" + (m_run && m_run->IsPaused() ? "1" : "0") + "~" +
        (friendBonk && friendBonk->m_coopTier ? "1" : "0");
    if (line == m_coopStatus)
        return;
    m_coopStatus = line;
    SendAddon(line);
}

bool BonkPlayer::StartCoopRun(BonkTier const& tier, std::string& error, BonkRunChoice const& choice)
{
    Player* other = GetCoopFriend();
    if (!other || !IsAtCamp() || !other->GetBonkPlayer()->IsAtCamp() || !other->GetBonkPlayer()->IsAddonReady())
    {
        error = "Wait for your friend to choose a hero and arrive at the outpost.";
        return false;
    }
    BonkPlayer* friendBonk = other->GetBonkPlayer();
    if (!m_player->IsAlive() || !other->IsAlive() || m_player->IsBeingTeleported() || other->IsBeingTeleported())
    {
        error = "Wait until both heroes are ready at the outpost.";
        return false;
    }
    if (m_player->GetGroup() == nullptr || other->GetGroup() != m_player->GetGroup())
    {
        error = "Your party is being prepared. Try again in a moment.";
        return false;
    }
    if (!friendBonk->m_coopTier)
    {
        if (!CanStartRun(tier, error, choice) || !friendBonk->CanStartRun(tier, error, choice))
        {
            error = std::string(other->GetName()) + ": " + error;
            return false;
        }
        m_coopTier = tier.id;
        m_coopStartMap = choice.startMap ? choice.startMap->id : 0;
        m_coopDungeon = choice.dungeon ? choice.dungeon->id : 0;
        m_coopInvade = choice.invade;
        m_coopReadyMs = 0;
        Notify("Ready for " + tier.name + ". Waiting for " + other->GetName() + ".");
        friendBonk->Notify(std::string(m_player->GetName()) + " is ready for " + tier.name + ". Visit the Run Master and press Ready together.");
        SendCoopStatus();
        friendBonk->SendCoopStatus();
        return true;
    }

    BonkTier const* proposed = sBonkMgr.GetTier(friendBonk->m_coopTier);
    BonkRunChoice agreed;
    agreed.startMap = sBonkMgr.GetMap(friendBonk->m_coopStartMap);
    agreed.dungeon = sBonkMgr.GetMap(friendBonk->m_coopDungeon);
    agreed.invade = friendBonk->m_coopInvade;
    if (!proposed || !CanStartRun(*proposed, error, agreed) || !friendBonk->CanStartRun(*proposed, error, agreed))
    {
        friendBonk->m_coopTier = 0;
        return false;
    }
    m_coopTier = friendBonk->m_coopTier = 0;
    TrimPoolToggles();
    friendBonk->TrimPoolToggles();
    friendBonk->m_run.reset(new BonkRun(other, *proposed, agreed));
    m_run.reset(new BonkRun(m_player, *proposed, agreed));
    friendBonk->m_run->LinkCoop(m_player->GetObjectGuid(), false);
    m_run->LinkCoop(other->GetObjectGuid(), true);
    if (!friendBonk->m_run->Begin(error) || !m_run->Begin(error))
    {
        friendBonk->m_run->Abort();
        m_run->Abort();
        friendBonk->m_run.reset();
        m_run.reset();
        friendBonk->TeleportToCamp();
        TeleportToCamp();
        return false;
    }
    return true;
}

void BonkRun::LinkCoop(ObjectGuid partner, bool follower)
{
    m_coopPartner = partner;
    m_coopFollower = follower;
}

BonkRun* BonkRun::GetCoopPartner() const
{
    if (m_coopPartner.IsEmpty())
        return nullptr;
    Player* player = ObjectAccessor::FindPlayerNotInWorld(m_coopPartner);
    BonkRun* run = player ? player->GetBonkPlayer()->GetRun() : nullptr;
    return run && run->m_coopPartner == m_player->GetObjectGuid() ? run : nullptr;
}

bool BonkRun::OwnsEnemy(ObjectGuid owner) const
{
    return owner == m_player->GetObjectGuid() || (IsCoop() && owner == m_coopPartner);
}

Player* BonkRun::GetCoopTarget(Creature const* mob) const
{
    BonkRun* partner = GetCoopPartner();
    Player* current = m_player;
    if (partner && mob->GetVictim() == partner->m_player)
        current = partner->m_player;
    Player* candidate = partner && current == m_player ? partner->m_player : m_player;
    if (candidate->IsAlive() && candidate->GetMap() == mob->GetMap() &&
        (!current->IsAlive() || mob->GetDistance(candidate) + 4.0f < mob->GetDistance(current)))
        return candidate;
    return current;
}

void BonkRun::SyncCoop()
{
    if (!IsCoop())
        return;
    BonkRun* partner = GetCoopPartner();
    if (!partner || partner->IsFinished())
    {
        if (m_state == STATE_RUNNING || m_state == STATE_PORTAL)
        {
            m_player->GetBonkPlayer()->Notify("Your friend left. Returning to the outpost with your progress saved.");
            Finish(false);
        }
        return;
    }
    if (m_coopFollower && partner->m_player->GetMap() == m_player->GetMap())
    {
        m_mobs = partner->m_mobs;
        m_elapsedMs = partner->m_elapsedMs;
        m_bossGuid = partner->m_bossGuid;
        m_bossSpawned = partner->m_bossSpawned;
        m_waveIndex = partner->m_waveIndex;
    }
    RefreshCoopPause();
}

void BonkRun::RefreshCoopPause()
{
    BonkRun* partner = GetCoopPartner();
    bool const entering = partner && (m_state == STATE_ENTERING || partner->m_state == STATE_ENTERING);
    bool const paused = m_coopPauseWanted || entering || (partner && partner->m_coopPauseWanted);
    SetPausedLocal(paused);
    if (partner)
        partner->SetPausedLocal(paused);
}

void BonkRun::EndCoop(bool wentHome, bool abort)
{
    if (!IsCoop() || m_coopEnding)
        return;
    m_coopEnding = true;
    if (BonkRun* partner = GetCoopPartner())
    {
        partner->m_coopEnding = true;
        partner->m_coopPauseWanted = false;
        partner->m_menuOpen = false;
        if (abort)
            partner->Surrender();
        else
            partner->Finish(wentHome);
        partner->m_player->GetBonkPlayer()->Notify(abort ? "Your friend disconnected. The run has ended." : "Returning to the outpost together.");
    }
}

void BonkRun::CreditCoopKill(Creature* mob, BonkSpawnKind kind, Unit* killer)
{
    if (m_coopFollower)
        return;
    if (BonkRun* partner = GetCoopPartner())
    {
        partner->m_bossGuid = kind == BONK_SPAWN_BOSS && m_state == STATE_PORTAL ? mob->GetObjectGuid() : m_bossGuid;
        partner->m_bossArrivals = m_bossArrivals;
        partner->OnMobKilled(mob, kind, killer);
    }
}
