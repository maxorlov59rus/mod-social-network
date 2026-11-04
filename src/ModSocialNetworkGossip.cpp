/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SocialNetwork.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "GossipDef.h"
#include "ScriptedGossip.h"
#include "WorldSessionMgr.h"

using namespace Socn;

namespace
{
    // NPC entry of the Social Network terminal, seeded by data/sql/db-world/social_network_world.sql
    constexpr uint32 SOCN_TERMINAL_NPC = 911100;

    // Gossip actions (encoded in the menu option action field)
    enum SocnGossipAction
    {
        GOSSIP_ACTION_PROFILE = 1,
        GOSSIP_ACTION_WALL = 2,
        GOSSIP_ACTION_INBOX = 3,
        GOSSIP_ACTION_TOP = 4,
        GOSSIP_ACTION_HELP = 5
    };

    class SocnTerminalCreature : public CreatureScript
    {
    public:
        SocnTerminalCreature() : CreatureScript("SocnTerminalCreature") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            ClearGossipMenuFor(player);

            WorldSession* session = player->GetSession();
            if (!session || !session->HasPermission(PERM_COMMAND))
                return false;

            ChatHandler handler(session);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GetSocnString(&handler, SOCN_GOSSIP_PROFILE),
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_PROFILE);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, GetSocnString(&handler, SOCN_GOSSIP_WALL),
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_WALL);
            AddGossipItemFor(player, GOSSIP_ICON_TABARD, GetSocnString(&handler, SOCN_GOSSIP_INBOX),
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INBOX);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, GetSocnString(&handler, SOCN_GOSSIP_TOP),
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TOP);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, GetSocnString(&handler, SOCN_GOSSIP_HELP),
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_HELP);

            SendGossipMenuFor(player, creature->GetEntry(), creature);
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
        {
            ClearGossipMenuFor(player);
            CloseGossipMenuFor(player);

            ChatHandler handler(player->GetSession());

            switch (action)
            {
                case GOSSIP_ACTION_PROFILE:
                    SendSocnMessage(&handler, SOCN_PROFILE_HEADER, player->GetName());
                    PrintProfileSummary(&handler, player->GetGUID());
                    break;
                case GOSSIP_ACTION_WALL:
                    ShowWall(&handler, player->GetGUID(), player->GetName());
                    break;
                case GOSSIP_ACTION_INBOX:
                    ShowInbox(&handler, player);
                    break;
                case GOSSIP_ACTION_TOP:
                    ShowTop(&handler);
                    break;
                case GOSSIP_ACTION_HELP:
                    SendSocnMessage(&handler, SOCN_GOSSIP_HELP);
                    break;
                default:
                    break;
            }

            return true;
        }

    private:
        // Shared read-only views (duplicated from chat handlers intentionally to
        // keep the gossip script self-contained without coupling to command internals)
        static void PrintProfileSummary(ChatHandler* handler, ObjectGuid targetGuid)
        {
            ObjectGuid::LowType lowGuid = targetGuid.GetCounter();

            uint32 guildId = 0;
            uint32 level = 0;
            uint32 playTime = 0;
            std::string status;

            // Guild membership is stored in guild_member (no guildId column
            // in `characters`), so LEFT JOIN resolves it (0 if none).
            if (QueryResult result = CharacterDatabase.Query(
                "SELECT c.`level`, c.`totaltime`, IFNULL(gm.`guildid`, 0) FROM `characters` c"
                " LEFT JOIN `guild_member` gm ON gm.`guid` = c.`guid` WHERE c.`guid` = {}", lowGuid))
            {
                level = result->Fetch()[0].Get<uint8>();
                playTime = result->Fetch()[1].Get<uint32>();
                guildId = result->Fetch()[2].Get<uint32>();
            }

            SendSocnMessage(handler, SOCN_PROFILE_LEVEL, level);
            SendSocnMessage(handler, SOCN_PROFILE_PLAYTIME, secsToTimeString(playTime));

            if (QueryResult result = CharacterDatabase.Query(
                "SELECT `status` FROM `socn_profiles` WHERE `guid` = {}", lowGuid))
                status = result->Fetch()[0].Get<std::string>();

            if (guildId)
            {
                if (QueryResult guild = CharacterDatabase.Query("SELECT `name` FROM `guild` WHERE `guildid` = {}", guildId))
                    SendSocnMessage(handler, SOCN_PROFILE_GUILD, guild->Fetch()[0].Get<std::string>());
                else
                    SendSocnMessage(handler, SOCN_PROFILE_GUILD_NONE);
            }
            else
                SendSocnMessage(handler, SOCN_PROFILE_GUILD_NONE);

            if (status.empty())
                SendSocnMessage(handler, SOCN_PROFILE_STATUS_NONE);
            else
                SendSocnMessage(handler, SOCN_PROFILE_STATUS, status);

            uint32 likes = 0;
            if (QueryResult result = CharacterDatabase.Query(
                "SELECT COUNT(*) FROM `socn_post_likes` l JOIN `socn_posts` p ON p.`id` = l.`postId` WHERE p.`authorGuid` = {}", lowGuid))
                likes = result->Fetch()[0].Get<uint32>();
            SendSocnMessage(handler, SOCN_PROFILE_LIKES, likes);

            if (QueryResult result = CharacterDatabase.Query(
                "SELECT (SELECT COUNT(*) FROM `socn_follows` WHERE `targetGuid` = {}),"
                " (SELECT COUNT(*) FROM `socn_follows` WHERE `followerGuid` = {})", lowGuid, lowGuid))
            {
                SendSocnMessage(handler, SOCN_PROFILE_FOLLOWERS, result->Fetch()[0].Get<uint32>());
                SendSocnMessage(handler, SOCN_PROFILE_FOLLOWING, result->Fetch()[1].Get<uint32>());
            }
        }

        static void ShowWall(ChatHandler* handler, ObjectGuid targetGuid, std::string const& targetName)
        {
            SendSocnMessage(handler, SOCN_WALL_HEADER, targetName, g_wallPageSize);

            QueryResult result = CharacterDatabase.Query(
                "SELECT p.`id`, p.`text`, p.`repostOf`,"
                " IFNULL(o.`authorGuid`, 0) FROM `socn_posts` p"
                " LEFT JOIN `socn_posts` o ON o.`id` = p.`repostOf`"
                " WHERE p.`authorGuid` = {} ORDER BY p.`id` DESC LIMIT {}",
                targetGuid.GetCounter(), g_wallPageSize);

            if (!result)
            {
                SendSocnMessage(handler, SOCN_WALL_EMPTY);
                return;
            }

            do
            {
                Field* fields = result->Fetch();
                uint32 repostOf = fields[2].Get<uint32>();
                if (repostOf)
                {
                    ObjectGuid origAuthor = ObjectGuid::Create<HighGuid::Player>(fields[3].Get<uint32>());
                    SendSocnMessage(handler, SOCN_WALL_REPOST_ENTRY, fields[0].Get<uint32>(),
                        fields[1].Get<std::string>(), GetPlayerNameByGuid(origAuthor));
                }
                else
                    SendSocnMessage(handler, SOCN_WALL_ENTRY, fields[0].Get<uint32>(), fields[1].Get<std::string>());
            } while (result->NextRow());
        }

        static void ShowInbox(ChatHandler* handler, Player* player)
        {
            QueryResult result = CharacterDatabase.Query(
                "SELECT `id`, `senderGuid`, `text` FROM `socn_messages`"
                " WHERE `receiverGuid` = {} ORDER BY `id` DESC LIMIT {}",
                player->GetGUID().GetCounter(), g_inboxPageSize);

            if (!result)
            {
                SendSocnMessage(handler, SOCN_INBOX_EMPTY);
                return;
            }

            do
            {
                Field* fields = result->Fetch();
                ObjectGuid senderGuid = ObjectGuid::Create<HighGuid::Player>(fields[1].Get<uint32>());
                SendSocnMessage(handler, SOCN_INBOX_ENTRY,
                    fields[0].Get<uint32>(), GetPlayerNameByGuid(senderGuid), fields[2].Get<std::string>());
            } while (result->NextRow());
        }

        static void ShowTop(ChatHandler* handler)
        {
            SendSocnMessage(handler, SOCN_TOP_HEADER, g_topSize);

            QueryResult result = CharacterDatabase.Query(
                "SELECT p.`authorGuid`, COUNT(*) AS `likes` FROM `socn_post_likes` l"
                " JOIN `socn_posts` p ON p.`id` = l.`postId`"
                " GROUP BY p.`authorGuid` ORDER BY `likes` DESC LIMIT {}", g_topSize);

            if (!result)
            {
                SendSocnMessage(handler, SOCN_TOP_EMPTY);
                return;
            }

            uint32 rank = 0;
            do
            {
                Field* fields = result->Fetch();
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(fields[0].Get<uint32>());
                SendSocnMessage(handler, SOCN_TOP_ENTRY, ++rank, GetPlayerNameByGuid(guid), fields[1].Get<uint32>());
            } while (result->NextRow());
        }
    };
}
