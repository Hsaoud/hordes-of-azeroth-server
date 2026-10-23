#include "BonkPlayer.h"
#include "BonkRun.h"
#include "BonkMobAI.h"
#include "Config/Config.h"
#include "Creature.h"
#include "GameObject.h"
#include "Group.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include <cmath>

namespace
{
    std::atomic<uint32> nextCoopToken{0};
    uint32 NewCoopToken() { return ++nextCoopToken; }
}

std::vector<Player*> BonkPlayer::GetCoopGroup() const
{
    std::vector<Player*> members{m_player};
    if (Group* group = m_player->GetGroup())
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            if (Player* player = ref->getSource())
                if (player != m_player && player->IsInWorld())
                    members.push_back(player);
    return members;
}

void BonkPlayer::CancelCoopProposal()
{
    if (m_coopTier)
    {
        for (auto const& member : m_coopInvited)
            if (Player* player = ObjectAccessor::FindPlayerNotInWorld(member.first))
            {
                BonkPlayer* bonk = player->GetBonkPlayer();
                if (bonk->m_coopProposer == m_player->GetObjectGuid())
                {
                    bonk->m_coopProposer.Clear();
                    bonk->SendAddon("coopprompt~0");
                }
            }
        m_coopTier = 0;
        m_coopInvited.clear();
    }
    else if (!m_coopProposer.IsEmpty())
    {
        std::string error;
        AnswerCoop(m_coopToken, false, error);
    }
    SendCoopStatus();
}

void BonkPlayer::OfferCoopJoin()
{
    if (m_run || m_coopTier || !IsAtCamp() || !m_coopProposer.IsEmpty())
        return;
    for (Player* member : GetCoopGroup())
        if (BonkRun* run = member->GetBonkPlayer()->GetRun())
            if (run->IsCoop() && (run->IsActive() || run->IsPortalOpen()) && run->GetCoopToken() != m_coopDismissed)
            {
                BonkRun* leader = run->GetCoopLeader();
                if (!leader) continue;
                m_coopProposer = leader->GetHero()->GetObjectGuid();
                m_coopToken = run->GetCoopToken();
                m_coopJoining = true;
                break;
            }
}

void BonkPlayer::UpdateCoop(uint32 diff)
{
    if (!sConfig.GetBoolDefault("Bonk.Coop", false)) return;
    m_coopTimer += diff;
    if (m_coopTier) m_coopReadyMs += diff;
    if (m_coopTimer < 500) return;
    m_coopTimer = 0;
    if (m_coopTier)
    {
        if (!IsAtCamp() || !m_player->IsAlive() || m_run)
            CancelCoopProposal();
        else
        {
            bool waiting = false;
            for (auto& entry : m_coopInvited)
            {
                Player* member = ObjectAccessor::FindPlayer(entry.first);
                if (!member || member->GetGroup() != m_player->GetGroup() || !member->GetBonkPlayer()->IsAtCamp() ||
                    (m_coopReadyMs >= 45000 && entry.second == 0))
                    entry.second = 2;
                waiting |= entry.second == 0;
            }
            if (!waiting) LaunchCoopProposal();
        }
    }
    OfferCoopJoin();
    SendCoopStatus();
}

void BonkPlayer::SendCoopStatus()
{
    if (!IsAddonReady()) return;
    uint32 ready = m_coopTier ? 1 : 0;
    for (auto const& entry : m_coopInvited) if (entry.second == 1) ++ready;
    uint32 const members = m_run && m_run->IsCoop() ? uint32(m_run->GetCoopRuns().size()) : uint32(GetCoopGroup().size());
    std::string line = "coop~" + std::to_string(members) + "~" + std::to_string(ready) + "~" +
        std::to_string(m_coopTier ? m_coopInvited.size() + 1 : 0) + "~" + (m_run && m_run->IsPaused() ? "1" : "0") + "~" +
        std::to_string(m_coopTier ? (45000 - std::min(m_coopReadyMs, 45000u)) / 1000 : 0);
    if (line != m_coopStatus) { m_coopStatus = line; SendAddon(line); }
    if (m_coopProposer.IsEmpty()) return;
    Player* proposer = ObjectAccessor::FindPlayer(m_coopProposer);
    BonkPlayer* bonk = proposer ? proposer->GetBonkPlayer() : nullptr;
    BonkRun* run = bonk ? bonk->GetRun() : nullptr;
    bool const valid = proposer && m_player->GetGroup() && m_player->GetGroup() == proposer->GetGroup() &&
        (m_coopJoining ? run && run->IsCoop() && run->GetCoopToken() == m_coopToken && (run->IsActive() || run->IsPortalOpen()) :
        bonk->m_coopTier && bonk->m_coopToken == m_coopToken);
    if (!valid)
    {
        m_coopProposer.Clear();
        SendAddon("coopprompt~0");
        return;
    }
    BonkTier const* tier = m_coopJoining ? &run->GetTier() : sBonkMgr.GetTier(bonk->m_coopTier);
    SendAddon("coopprompt~" + std::to_string(m_coopToken) + "~" + proposer->GetName() + "~" + (tier ? tier->name : "") + "~" +
        (m_coopJoining ? run->GetMap().name : "New expedition") + "~" + (m_coopJoining ? "join" : "start"));
}

bool BonkPlayer::StartCoopRun(BonkTier const& tier, std::string& error, BonkRunChoice const& choice)
{
    if (!IsAtCamp() || !m_player->IsAlive() || m_player->IsBeingTeleported())
    {
        error = "Return to the Inn before proposing a new expedition.";
        return false;
    }
    if (!CanStartRun(tier, error, choice)) return false;
    if (IsCoopReady()) { error = "Answer or cancel the current invitation first."; return false; }
    m_coopTier = tier.id;
    m_coopStartMap = choice.startMap ? choice.startMap->id : 0;
    m_coopDungeon = choice.dungeon ? choice.dungeon->id : 0;
    m_coopInvade = choice.invade;
    m_coopReadyMs = 0;
    m_coopToken = NewCoopToken();
    m_coopInvited.clear();
    for (Player* member : GetCoopGroup())
    {
        if (member == m_player) continue;
        BonkPlayer* bonk = member->GetBonkPlayer();
        if (!bonk->IsAtCamp() || bonk->GetRun() || !bonk->IsAddonReady() || bonk->IsCoopReady()) continue;
        bonk->m_coopProposer = m_player->GetObjectGuid();
        bonk->m_coopToken = m_coopToken;
        bonk->m_coopJoining = false;
        m_coopInvited[member->GetObjectGuid()] = 0;
        bonk->SendCoopStatus();
    }
    if (m_coopInvited.empty()) LaunchCoopProposal();
    else Notify("Expedition proposed. Your group can accept from anywhere in the Inn.");
    SendCoopStatus();
    return true;
}

bool BonkPlayer::AnswerCoop(uint32 token, bool accept, std::string& error)
{
    if (token != m_coopToken || m_coopProposer.IsEmpty()) { error = "This invitation has expired."; return false; }
    Player* proposer = ObjectAccessor::FindPlayer(m_coopProposer);
    if (!proposer || !m_player->GetGroup() || m_player->GetGroup() != proposer->GetGroup())
    {
        m_coopProposer.Clear();
        SendAddon("coopprompt~0");
        error = "The expedition is no longer available.";
        return false;
    }
    BonkPlayer* lead = proposer->GetBonkPlayer();
    if (m_coopJoining)
    {
        BonkRun* run = lead->GetRun();
        if (accept)
        {
            BonkRunChoice choice = run ? run->GetCoopChoice() : BonkRunChoice();
            if (!run || !run->IsCoop() || run->GetCoopToken() != token || !IsAtCamp() || !m_player->IsAlive() || m_player->IsBeingTeleported() ||
                !CanStartRun(run->GetTier(), error, choice))
            { if (error.empty()) error = "You cannot join this expedition right now."; return false; }
            TrimPoolToggles();
            m_run.reset(new BonkRun(m_player, run->GetTier()));
            if (!m_run->JoinCoop(*run, error)) { m_run->Abort(); m_run.reset(); return false; }
        }
        m_coopDismissed = token;
    }
    else
    {
        auto invited = lead->m_coopInvited.find(m_player->GetObjectGuid());
        if (!lead->m_coopTier || lead->m_coopToken != token || invited == lead->m_coopInvited.end())
        { error = "This invitation has expired."; return false; }
        BonkTier const* tier = sBonkMgr.GetTier(lead->m_coopTier);
        BonkRunChoice choice;
        choice.startMap = sBonkMgr.GetMap(lead->m_coopStartMap);
        choice.dungeon = sBonkMgr.GetMap(lead->m_coopDungeon);
        choice.invade = lead->m_coopInvade;
        if (accept && (!tier || !IsAtCamp() || !m_player->IsAlive() || m_player->IsBeingTeleported() || !CanStartRun(*tier, error, choice)))
        { if (error.empty()) error = "You cannot start this expedition right now."; return false; }
        invited->second = accept ? 1 : 2;
        if (!accept) m_coopDismissed = token;
    }
    m_coopProposer.Clear();
    SendAddon("coopprompt~0");
    lead->SendCoopStatus();
    return true;
}

void BonkPlayer::LaunchCoopProposal()
{
    BonkTier const* tier = sBonkMgr.GetTier(m_coopTier);
    BonkRunChoice choice;
    choice.startMap = sBonkMgr.GetMap(m_coopStartMap);
    choice.dungeon = sBonkMgr.GetMap(m_coopDungeon);
    choice.invade = m_coopInvade;
    std::string error;
    if (!tier || !CanStartRun(*tier, error, choice)) { CancelCoopProposal(); Notify(error); return; }
    std::vector<BonkPlayer*> ready{this};
    for (auto const& entry : m_coopInvited)
        if (Player* member = ObjectAccessor::FindPlayer(entry.first))
        {
            BonkPlayer* bonk = member->GetBonkPlayer();
            bonk->m_coopProposer.Clear();
            bonk->SendAddon("coopprompt~0");
            if (entry.second == 1 && member->GetGroup() == m_player->GetGroup() && member->IsAlive() && !member->IsBeingTeleported() &&
                bonk->IsAtCamp() && bonk->CanStartRun(*tier, error, choice)) ready.push_back(bonk);
        }
    m_coopTier = 0;
    m_coopInvited.clear();
    for (BonkPlayer* bonk : ready)
    {
        bonk->TrimPoolToggles();
        bonk->m_run.reset(new BonkRun(bonk->m_player, *tier, choice));
        bonk->m_run->LinkCoop(*m_run);
        bonk->m_run->SetCoopToken(m_coopToken);
        bonk->m_player->SetFactionTemplateId(m_player->GetFactionTemplateId());
    }
    for (BonkPlayer* bonk : ready)
        if (!bonk->m_run->Begin(error))
        {
            for (BonkPlayer* failed : ready) { failed->m_run->Abort(); failed->m_run.reset(); failed->TeleportToCamp(); failed->Notify(error); }
            return;
        }
}

void BonkRun::LinkCoop(BonkRun& leader)
{
    if (!leader.m_coop)
    {
        leader.m_coop = std::make_shared<CoopMission>();
        leader.m_coop->leader = leader.m_player->GetObjectGuid();
        leader.m_coop->token = NewCoopToken();
        leader.m_coop->members.push_back(leader.m_player->GetObjectGuid());
    }
    m_coop = leader.m_coop;
    if (std::find(m_coop->members.begin(), m_coop->members.end(), m_player->GetObjectGuid()) == m_coop->members.end())
        m_coop->members.push_back(m_player->GetObjectGuid());
}

BonkRunChoice BonkRun::GetCoopChoice() const
{
    BonkRunChoice choice;
    choice.startMap = sBonkMgr.GetMap(m_startMapId);
    choice.dungeon = sBonkMgr.GetMap(m_dungeonId ? m_dungeonId : m_invaderId);
    choice.invade = m_invaderId != 0;
    return choice;
}

bool BonkRun::IsCoopFollower() const { return m_coop && m_coop->leader != m_player->GetObjectGuid(); }
uint32 BonkRun::GetCoopToken() const { return m_coop ? m_coop->token : 0; }

BonkRun* BonkRun::GetCoopLeader() const
{
    if (!m_coop) return nullptr;
    Player* player = ObjectAccessor::FindPlayerNotInWorld(m_coop->leader);
    BonkRun* run = player ? player->GetBonkPlayer()->GetRun() : nullptr;
    return run && run->m_coop == m_coop ? run : nullptr;
}

std::vector<BonkRun*> BonkRun::GetCoopRuns() const
{
    std::vector<BonkRun*> runs;
    if (!m_coop) return runs;
    for (ObjectGuid guid : m_coop->members)
        if (Player* player = ObjectAccessor::FindPlayerNotInWorld(guid))
            if (BonkRun* run = player->GetBonkPlayer()->GetRun())
                if (run->m_coop == m_coop && run->m_state != STATE_ENDING && run->m_state != STATE_FINISHED) runs.push_back(run);
    return runs;
}

bool BonkRun::OwnsEnemy(ObjectGuid owner) const
{
    return owner == m_player->GetObjectGuid() || (m_coop && std::find(m_coop->members.begin(), m_coop->members.end(), owner) != m_coop->members.end());
}

Player* BonkRun::GetCoopTarget(Creature const* mob) const
{
    Player* target = m_player;
    for (BonkRun* run : GetCoopRuns())
    {
        Player* hero = run->m_player;
        if (!hero->IsAlive() || hero->GetMap() != mob->GetMap() || hero->IsBeingTeleported()) continue;
        if (!target->IsAlive() || target->GetMap() != mob->GetMap() || mob->GetDistance(hero) + 4.0f < mob->GetDistance(target)) target = hero;
    }
    return target;
}

float BonkRun::GetCoopHealthScale() const { return 1.0f + 0.7f * float(std::max(size_t(1), GetCoopRuns().size()) - 1); }

void BonkRun::RefreshCoopScaling()
{
    if (!m_coop || IsCoopFollower()) return;
    float const scale = GetCoopHealthScale();
    if (std::fabs(scale - m_coop->healthScale) < 0.001f) return;
    float const ratio = scale / m_coop->healthScale;
    for (ObjectGuid guid : m_mobs)
        if (Creature* mob = m_player->GetMap()->GetCreature(guid))
            if (mob->IsAlive())
            {
                uint32 const maximum = std::max(1u, uint32(std::min(double(UINT32_MAX), double(mob->GetMaxHealth()) * ratio)));
                uint32 const health = std::max(1u, uint32(double(maximum) * mob->GetHealth() / std::max(1u, mob->GetMaxHealth())));
                mob->SetMaxHealth(maximum);
                mob->SetHealth(std::min(maximum, health));
            }
    m_coop->healthScale = scale;
}

void BonkRun::SyncCoop()
{
    if (!m_coop) return;
    if (BonkRun* leader = GetCoopLeader())
        if (leader != this && leader->m_player->GetMap() == m_player->GetMap() && m_state != STATE_ENTERING)
        {
            m_mobs = leader->m_mobs;
            m_elapsedMs = leader->m_elapsedMs;
            m_bossGuid = leader->m_bossGuid;
            m_bossSpawned = leader->m_bossSpawned;
            m_waveIndex = leader->m_waveIndex;
        }
    RefreshCoopScaling();
    RefreshCoopPause();
}

void BonkRun::RefreshCoopPause()
{
    bool paused = false;
    auto runs = GetCoopRuns();
    for (BonkRun* run : runs) paused |= (run->m_player->IsAlive() && run->m_coopPauseWanted) || run->m_state == STATE_ENTERING;
    for (BonkRun* run : runs) run->SetPausedLocal(paused);
}

bool BonkRun::JoinCoop(BonkRun& source, std::string& error)
{
    BonkRun* leader = source.GetCoopLeader();
    if (!leader || (!leader->IsActive() && !leader->IsPortalOpen())) { error = "This expedition has ended."; return false; }
    LinkCoop(*leader);
    m_coopLateJoin = true;
    m_stage = leader->m_stage;
    m_dungeonId = leader->m_dungeonId;
    m_invaderId = leader->m_invaderId;
    m_totalMs = leader->m_totalMs;
    m_player->SetFactionTemplateId(leader->m_player->GetFactionTemplateId());
    return Begin(error);
}

void BonkRun::CompleteCoopArrival()
{
    if (!IsCoopFollower()) return;
    m_coopLateJoin = false;
    BonkRun* leader = GetCoopLeader();
    if (!leader || leader == this) return;
    for (uint32 i = 0; i < 100 && m_player->GetLevel() < leader->m_player->GetLevel(); ++i)
    {
        uint32 const next = GetNextLevelXp(), have = m_player->GetUInt32Value(PLAYER_XP);
        if (next <= have) break;
        GiveExperience(next - have);
    }
    m_player->SetUInt32Value(PLAYER_XP, leader->m_player->GetUInt32Value(PLAYER_XP));
    m_pendingLevels += leader->m_overcapLevel > m_overcapLevel ? leader->m_overcapLevel - m_overcapLevel : 0;
    m_overcapLevel = leader->m_overcapLevel;
    m_overcapXp = leader->m_overcapXp;
    UpdateRunXp(true);
    SyncCoop();
    if (leader->IsPortalOpen())
    {
        m_state = STATE_PORTAL;
        m_nextMapId = leader->m_nextMapId;
        PlacePortal(m_player->GetPositionX() + 3.0f, m_player->GetPositionY(), m_player->GetPositionZ());
    }
    m_player->GetBonkPlayer()->Notify("You joined the expedition. Choose your upgrades, then fight alongside your party.");
}

bool BonkRun::HasCoopChoice() const
{
    return !m_offer.empty() || !m_leyOffer.empty() || m_gearOfferEntry || !m_glyphOffer.empty() || m_vendorOpen || m_vaultOpen || m_menuOpen;
}

void BonkRun::ReadyCoopPortal()
{
    m_coopPortalReady = true;
    auto runs = GetCoopRuns();
    for (BonkRun* run : runs)
        if (!run->m_coopPortalReady || run->m_state != STATE_PORTAL || run->HasCoopChoice())
        {
            m_player->GetBonkPlayer()->Notify("Ready for the next stage. Waiting for the rest of the expedition.");
            return;
        }
    BonkRun* leader = GetCoopLeader();
    if (!leader) return;
    m_coop->advancing = true;
    leader->EnterNextStage();
    if (leader->m_state == STATE_ENTERING)
        for (BonkRun* run : runs) if (run != leader) run->EnterNextStage();
    m_coop->advancing = false;
}

void BonkRun::TransferCoopWorld(BonkRun& next)
{
    m_telegraphs.Clear(m_player);
    next.m_orbs = std::move(m_orbs);
    next.m_orbMerges = std::move(m_orbMerges);
    next.m_orbMergeTimer = m_orbMergeTimer;
    next.m_lastOrbMergeId = m_lastOrbMergeId;
    next.m_powerups = std::move(m_powerups);
    next.m_powerupObjectives = std::move(m_powerupObjectives);
    next.m_powerupSpawnTimer = m_powerupSpawnTimer;
    next.m_chests = std::move(m_chests);
    next.m_chestChoices = std::move(m_chestChoices);
    next.m_chestObjectives = std::move(m_chestObjectives);
    next.m_mobs = std::move(m_mobs);
    next.m_objectives = std::move(m_objectives);
    m_objectives = BonkObjectives();
    next.m_miniEvent = std::move(m_miniEvent);
    next.m_hidden = std::move(m_hidden);
    m_hidden = HiddenEncounter();
    next.m_shrines = std::move(m_shrines);
    next.m_pendingSummons = std::move(m_pendingSummons);
    next.m_heldEvents = std::move(m_heldEvents);
    next.m_nextEvent = m_nextEvent;
    next.m_miniEventActive = m_miniEventActive;
    next.m_nextMiniEventMs = m_nextMiniEventMs;
    next.m_lastMiniEvent = m_lastMiniEvent;
    next.m_gruntRadius = m_gruntRadius;
    next.m_spawnTimer = m_spawnTimer;
    next.m_eliteTimer = m_eliteTimer;
    next.m_waveIndex = m_waveIndex;
    next.m_waveEntered = m_waveEntered;
    next.m_waveWarned = m_waveWarned;
    next.m_miniEventSlots = m_miniEventSlots;
    next.m_overtimeWaves = m_overtimeWaves;
    next.m_overtimeTimer = m_overtimeTimer;
    next.m_spawnDamageScale = m_spawnDamageScale;
    next.m_spawnHealthScale = m_spawnHealthScale;
    next.m_bossSpawned = m_bossSpawned;
    next.m_bossGuid = m_bossGuid;
    next.m_lastPlateId = m_lastPlateId;
    next.m_bossArrivals = m_bossArrivals;
    next.m_overtimeReported = m_overtimeReported;
    next.m_elapsedMs = m_elapsedMs;
    next.m_nextMapId = m_nextMapId;
    next.m_playedMaps = m_playedMaps;
    next.m_shrinesToPlace = m_shrinesToPlace;
    next.m_shrineTimer = m_shrineTimer;
    next.m_moonwellPlaced = m_moonwellPlaced;
    next.m_leyLinesPlaced = m_leyLinesPlaced;
    next.m_stonePlaced = m_stonePlaced;
    next.m_stoneAt = m_stoneAt;
    m_miniEventActive = false;
    for (ObjectGuid guid : next.m_mobs)
        if (Creature* mob = m_player->GetMap()->GetCreature(guid))
            if (BonkMobAI* ai = dynamic_cast<BonkMobAI*>(mob->AI())) ai->SetCoopOwner(next.m_player->GetObjectGuid());
}

void BonkRun::EndCoop(bool, bool)
{
    ClearCoopGrave();
    if (!m_coop) return;
    auto runs = GetCoopRuns();
    BonkRun* next = nullptr;
    for (BonkRun* run : runs) if (run != this) { next = run; break; }
    if (!IsCoopFollower() && next)
    {
        TransferCoopWorld(*next);
        m_coop->leader = next->m_player->GetObjectGuid();
    }
    if (next) m_mobs.clear();
    auto& members = m_coop->members;
    members.erase(std::remove(members.begin(), members.end(), m_player->GetObjectGuid()), members.end());
    m_coop.reset();
    m_coopPauseWanted = false;
    if (next)
    {
        next->RefreshCoopScaling();
        next->RefreshCoopPause();
        for (BonkRun* run : next->GetCoopRuns())
            run->m_player->GetBonkPlayer()->Notify(std::string(m_player->GetName()) + " left the expedition. Difficulty has adjusted to the remaining heroes.");
    }
}

void BonkRun::CreditCoopKill(Creature* mob, BonkSpawnKind kind, Unit* killer)
{
    if (IsCoopFollower()) return;
    for (BonkRun* member : GetCoopRuns())
        if (member != this)
        {
            member->m_bossGuid = kind == BONK_SPAWN_BOSS && m_state == STATE_PORTAL ? mob->GetObjectGuid() : m_bossGuid;
            member->m_bossArrivals = m_bossArrivals;
            member->OnMobKilled(mob, kind, killer);
        }
}

bool BonkRun::HoldCoopDeath() const
{
    return m_coop && !m_player->IsAlive() && (m_state == STATE_RUNNING || m_state == STATE_PORTAL);
}

void BonkRun::ClearCoopGrave()
{
    if (Map* map = m_player->FindMap())
        if (GameObject* grave = map->GetGameObject(m_coopGrave)) grave->Delete();
    m_coopGrave.Clear();
    m_coopRescueVisuals.Clear(m_player);
    m_coopRescueVisuals.Dismiss(m_player);
    m_coopRescueShown = false;
    if (BonkRun* leader = GetCoopLeader())
        if (m_coopGraveObjective) leader->RemoveObjective(m_coopGraveObjective);
    m_coopGraveObjective = 0;
    m_coopRescueMs = 0;
    m_coopRescueReportMs = 0;
    auto runs = GetCoopRuns();
    if (runs.empty()) runs.push_back(this);
    for (BonkRun* run : runs)
        run->m_player->GetBonkPlayer()->SendAddon("rescue~" + std::string(m_player->GetName()) + "~0~0~" + (run == this ? "1" : "0"));
}

bool BonkRun::UpdateCoopRescue(uint32 diff)
{
    if (!m_coop || (m_state != STATE_RUNNING && m_state != STATE_PORTAL)) return false;
    auto runs = GetCoopRuns();
    BonkRun* living = nullptr;
    for (BonkRun* run : runs)
        if (run->m_player->IsAlive() && run->m_state != STATE_ENTERING) { living = run; break; }
    if (!living)
    {
        for (BonkRun* run : runs) if (run->m_state == STATE_ENTERING) return HoldCoopDeath();
        for (BonkRun* run : runs) run->ClearCoopGrave();
        for (BonkRun* run : runs) run->Finish(false);
        return true;
    }
    if (!IsCoopFollower() && !m_player->IsAlive())
    {
        TransferCoopWorld(*living);
        m_coop->leader = living->m_player->GetObjectGuid();
    }
    if (!IsCoopFollower())
        for (BonkRun* run : runs)
        {
            Player* hero = run->m_player;
            if (!run->HoldCoopDeath())
            {
                if (!run->m_coopGrave.IsEmpty()) run->ClearCoopGrave();
                continue;
            }
            if (hero->GetMap() != m_player->GetMap() || hero->IsBeingTeleported()) continue;
            if (run->m_coopGrave.IsEmpty())
            {
                if (GameObject* grave = hero->SummonGameObject(37, hero->GetPositionX(), hero->GetPositionY(), hero->GetPositionZ(),
                    hero->GetOrientation(), 0, 0, 0, 0, 3600, false))
                {
                    grave->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                    run->m_coopGrave = grave->GetObjectGuid();
                }
                if (!run->m_coopGraveObjective)
                    run->m_coopGraveObjective = m_objectives.Add(m_player, BONK_OBJECTIVE_SHRINE, hero->GetPositionX(), hero->GetPositionY(),
                        hero->GetPositionZ(), std::string("Revive ") + hero->GetName(), 0, "Spell_Holy_Resurrection",
                        "Stay within 6 yards for 6 seconds to revive this ally.");
            }
            if (!run->m_coopRescueShown)
            {
                run->m_coopRescueVisuals.Add(hero, BONK_TELEGRAPH_CIRCLE, hero->GetPositionX(), hero->GetPositionY(),
                    hero->GetPositionZ(), 0, 6.0f, 3600000, nullptr, true);
                run->m_coopRescueShown = true;
            }
            run->m_coopRescueVisuals.Update(hero, diff);
            bool nearby = false;
            for (BonkRun* ally : runs)
                if (ally != run && ally->m_player->IsAlive() && ally->m_player->IsInWorld() && !ally->m_player->IsBeingTeleported() &&
                    ally->m_player->GetMap() == hero->GetMap() && hero->GetDistance3dToCenter(ally->m_player) <= 6.0f &&
                    hero->IsWithinLOSInMap(ally->m_player)) { nearby = true; break; }
            if (!nearby) run->m_coopRescueMs = 0;
            else if (!m_paused) run->m_coopRescueMs = std::min(6000u, run->m_coopRescueMs + diff);
            run->m_coopRescueReportMs += diff;
            if (run->m_coopRescueReportMs >= 200)
            {
                run->m_coopRescueReportMs = 0;
                for (BonkRun* member : runs)
                    member->m_player->GetBonkPlayer()->SendAddon("rescue~" + std::string(hero->GetName()) + "~" +
                        std::to_string(run->m_coopRescueMs) + "~6000~" + (member == run ? "1" : "0"));
            }
            if (run->m_coopRescueMs >= 6000)
            {
                hero->ResurrectPlayer(0.5f);
                hero->SpawnCorpseBones();
                hero->CastSpell(hero, 1022, true);
                run->ClearCoopGrave();
                run->UpdateOffers();
                for (BonkRun* member : runs)
                    member->m_player->GetBonkPlayer()->Notify(std::string(hero->GetName()) + " is back in the fight!");
            }
        }
    return HoldCoopDeath();
}

void BonkRun::GiveCoopExperience(uint32 xp)
{
    if (!m_coop) { GiveExperience(xp); return; }
    for (BonkRun* run : GetCoopRuns())
        if (run->m_state != STATE_ENTERING) run->GiveExperience(xp);
}
