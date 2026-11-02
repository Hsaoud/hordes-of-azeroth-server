#include "BonkLeaderboard.h"
#include "BonkPlayer.h"
#include "GameObject.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <sys/stat.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    char const* const LEADERBOARD_FILE = "leaderboard.txt";
    size_t const MAX_LEADERBOARD_LINES = 20000;
    size_t const MAX_LINE_LENGTH = 200;
    uint32 const PAGE_ROWS = 50;
    uint32 const SOLO_VIEW = 1;
    uint32 const LAST_GROUP_VIEW = 4;

    struct Row
    {
        std::string line;
        uint32 classId = 0;
        uint32 level = 0;
        uint32 ms = 0;
    };

    struct Board
    {
        time_t modified = 0;
        long long size = -1;
        std::vector<std::string> header;
        std::map<uint32, std::vector<Row>> solo;
        std::vector<Row> soloAll;
        std::map<uint32, std::vector<Row>> groups;
    };

    std::mutex s_boardLock;
    Board s_board;

    bool StartsWith(std::string const& line, char const* kind)
    {
        return line.compare(0, std::strlen(kind), kind) == 0;
    }

    bool IsBoardLine(std::string const& line)
    {
        if (line.empty() || line.size() > MAX_LINE_LENGTH || line.find('|') != std::string::npos)
            return false;
        for (char c : line)
            if (static_cast<unsigned char>(c) < 0x20)
                return false;
        for (char const* kind : { "updated~", "season~", "solo~", "group~", "mine~", "local~", "cutoff~", "sync~" })
            if (StartsWith(line, kind))
                return true;
        return false;
    }

    std::vector<std::string> Fields(std::string const& line)
    {
        std::vector<std::string> fields;
        size_t start = 0;
        while (true)
        {
            size_t const tilde = line.find('~', start);
            fields.push_back(line.substr(start, tilde == std::string::npos ? std::string::npos : tilde - start));
            if (tilde == std::string::npos)
                return fields;
            start = tilde + 1;
        }
    }

    uint32 Number(std::vector<std::string> const& fields, size_t index)
    {
        return index < fields.size() ? uint32(std::strtoul(fields[index].c_str(), nullptr, 10)) : 0;
    }

    bool Ahead(Row const& a, Row const& b)
    {
        return a.level != b.level ? a.level > b.level : a.ms < b.ms;
    }

    void Load(Board& board)
    {
        board = Board();
        std::ifstream file(LEADERBOARD_FILE);
        std::string line;
        size_t read = 0;
        while (file && read < MAX_LEADERBOARD_LINES && std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (!IsBoardLine(line))
                continue;
            ++read;
            std::vector<std::string> const fields = Fields(line);
            if (StartsWith(line, "solo~"))
            {
                Row row{ line, Number(fields, 1), Number(fields, 4), Number(fields, 5) };
                board.solo[row.classId].push_back(row);
                board.soloAll.push_back(row);
            }
            else if (StartsWith(line, "group~"))
                board.groups[Number(fields, 1)].push_back(Row{ line, 0, Number(fields, 3), Number(fields, 4) });
            else
                board.header.push_back(line);
        }
        std::stable_sort(board.soloAll.begin(), board.soloAll.end(), Ahead);
    }

    void Refresh()
    {
        struct stat info;
        if (stat(LEADERBOARD_FILE, &info) != 0)
        {
            if (s_board.size != -2)
            {
                s_board = Board();
                s_board.size = -2;
            }
            return;
        }
        if (info.st_mtime == s_board.modified && (long long)info.st_size == s_board.size)
            return;
        Load(s_board);
        s_board.modified = info.st_mtime;
        s_board.size = (long long)info.st_size;
    }

    bool HasMemberClass(std::string const& line, uint32 classId)
    {
        std::string const members = Fields(line).back();
        std::string const wanted = ":" + std::to_string(classId);
        size_t at = 0;
        while ((at = members.find(wanted, at)) != std::string::npos)
        {
            size_t const end = at + wanted.size();
            if (end == members.size() || members[end] == ',')
                return true;
            at = end;
        }
        return false;
    }

    std::vector<Row const*> ViewRows(uint32 view, uint32 classId)
    {
        std::vector<Row const*> rows;
        if (view == SOLO_VIEW)
        {
            if (!classId)
            {
                for (Row const& row : s_board.soloAll)
                    rows.push_back(&row);
                return rows;
            }
            auto found = s_board.solo.find(classId);
            if (found != s_board.solo.end())
                for (Row const& row : found->second)
                    rows.push_back(&row);
            return rows;
        }
        auto found = s_board.groups.find(view);
        if (found == s_board.groups.end())
            return rows;
        for (Row const& row : found->second)
            if (!classId || HasMemberClass(row.line, classId))
                rows.push_back(&row);
        return rows;
    }
}

namespace BonkLeaderboard
{
    void Send(Player* player)
    {
        BonkPlayer* bonk = player->GetBonkPlayer();
        bonk->SendAddon("boardopen");
        {
            std::lock_guard<std::mutex> guard(s_boardLock);
            Refresh();
            for (std::string const& line : s_board.header)
                bonk->SendAddon("board~" + line);
        }
        bonk->SendBoardChoice();
        bonk->SendAddon("boardend");
    }

    void SendPage(Player* player, uint32 view, uint32 classId, uint32 offset)
    {
        if (view < SOLO_VIEW || view > LAST_GROUP_VIEW)
            return;
        BonkPlayer* bonk = player->GetBonkPlayer();
        std::lock_guard<std::mutex> guard(s_boardLock);
        Refresh();
        std::vector<Row const*> const rows = ViewRows(view, classId);
        uint32 const total = uint32(rows.size());
        for (uint32 position = offset; position < total && position < offset + PAGE_ROWS; ++position)
            bonk->SendAddon("boardrow~" + std::to_string(view) + "~" + std::to_string(classId) + "~" + std::to_string(position + 1) + "~" + rows[position]->line);
        bonk->SendAddon("boardpage~" + std::to_string(view) + "~" + std::to_string(classId) + "~" + std::to_string(offset) + "~" + std::to_string(total));
    }
}

bool GOHello_go_bonk_board(Player* player, GameObject* /*go*/)
{
    if (player->GetBonkPlayer()->GetRun())
        return true;
    BonkLeaderboard::Send(player);
    return true;
}

void AddSC_bonk_leaderboard()
{
    Script* newscript = new Script;
    newscript->Name = "go_bonk_board";
    newscript->pGOHello = &GOHello_go_bonk_board;
    newscript->RegisterSelf();
}
