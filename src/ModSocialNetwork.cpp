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
#include "Chat.h"
#include "CommandScript.h"
#include "Config.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldScript.h"
#include <string>

using namespace Acore::ChatCommands;
using namespace Socn;

namespace
{
    class SocnCommandScript : public CommandScript
    {
    public:
        SocnCommandScript() : CommandScript("SocnCommandScript") { }

        ChatCommandTable GetCommands() const override
        {
            static ChatCommandTable socialCommandTable =
            {
                { "profile",  HandleSocnProfileCommand,  PERM_COMMAND,             Console::No },
                { "status",   HandleSocnStatusCommand,   PERM_COMMAND,             Console::No },
                { "post",     HandleSocnPostCommand,     PERM_COMMAND,             Console::No },
                { "repost",   HandleSocnRepostCommand,   PERM_COMMAND,             Console::No },
                { "wall",     HandleSocnWallCommand,     PERM_COMMAND,             Console::No },
                { "delete",   HandleSocnDeleteCommand,   PERM_COMMAND,             Console::No },
                { "like",     HandleSocnLikeCommand,     PERM_COMMAND,             Console::No },
                { "follow",   HandleSocnFollowCommand,   PERM_COMMAND,             Console::No },
                { "unfollow", HandleSocnUnfollowCommand, PERM_COMMAND,             Console::No },
                { "top",      HandleSocnTopCommand,      PERM_COMMAND,             Console::No },
                { "msg",      HandleSocnMsgCommand,      PERM_COMMAND,             Console::No },
                { "inbox",    HandleSocnInboxCommand,    PERM_COMMAND,             Console::No },
                { "msgdel",   HandleSocnMsgDelCommand,   PERM_COMMAND,             Console::No },
                { "comment",  HandleSocnCommentCommand,  PERM_COMMAND,             Console::No },
                { "comments", HandleSocnCommentsCommand, PERM_COMMAND,             Console::No },
                { "friend",   HandleSocnFriendCommand,   PERM_COMMAND,             Console::No },
                { "friendaccept",  HandleSocnFriendAcceptCommand,  PERM_COMMAND,      Console::No },
                { "frienddecline", HandleSocnFriendDeclineCommand, PERM_COMMAND,      Console::No },
                { "friends",  HandleSocnFriendsCommand,  PERM_COMMAND,             Console::No },
                { "requests", HandleSocnRequestsCommand, PERM_COMMAND,             Console::No },
                { "frienddel", HandleSocnFriendDelCommand, PERM_COMMAND,            Console::No },
                { "enemy",    HandleSocnEnemyCommand,    PERM_COMMAND,             Console::No },
                { "enemies",  HandleSocnEnemiesCommand,  PERM_COMMAND,             Console::No },
                { "enemydel", HandleSocnEnemyDelCommand, PERM_COMMAND,             Console::No },
                { "guild",    HandleSocnGuildCommand,    PERM_COMMAND,             Console::No },
                { "deleteany", HandleSocnDeleteAnyCommand, PERM_COMMAND_DELETE_ANY, Console::No }
            };
            static ChatCommandTable commandTable =
            {
                { "social", socialCommandTable }
            };
            return commandTable;
        }

    private:
        static void ShowProfile(ChatHandler* handler, ObjectGuid targetGuid)
        {
            ObjectGuid::LowType lowGuid = targetGuid.GetCounter();

            SendSocnMessage(handler, SOCN_PROFILE_HEADER, GetPlayerNameByGuid(targetGuid));

            uint32 guildId = 0;
            uint32 level = 0;
            uint32 playTime = 0;
            std::string status;

            // Basic character info lives in the core characters table;
            // guild membership is stored in guild_member (no guildId column
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

        static bool HandleSocnProfileCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = player->GetGUID();
            if (target)
            {
                ObjectGuid found = FindCharacterGuidByName(std::string(target->GetName()));
                if (found.IsEmpty())
                {
                    handler->SendErrorMessage("Character '{}' not found.", target->GetName());
                    return false;
                }
                targetGuid = found;
            }

            ShowProfile(handler, targetGuid);
            return true;
        }

        static bool HandleSocnStatusCommand(ChatHandler* handler, Tail text)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            std::string status{ text };
            if (!ValidateText(handler, status, g_maxStatusLen))
                return false;

            CharacterDatabase.Execute(
                "REPLACE INTO `socn_profiles` (`guid`, `status`) VALUES ({}, '{}')",
                player->GetGUID().GetCounter(), SqlEscape(status));

            SendSocnMessage(handler, SOCN_STATUS_SET);
            return true;
        }

        static bool HandleSocnPostCommand(ChatHandler* handler, Tail text)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            return PostToWall(player, std::string{ text }, handler);
        }

        static bool HandleSocnRepostCommand(ChatHandler* handler, uint32 postId)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            return RepostToWall(player, postId, handler);
        }

        static bool HandleSocnWallCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = player->GetGUID();
            if (target)
            {
                ObjectGuid found = FindCharacterGuidByName(std::string(target->GetName()));
                if (found.IsEmpty())
                {
                    handler->SendErrorMessage("Character '{}' not found.", target->GetName());
                    return false;
                }
                targetGuid = found;
            }

            SendSocnMessage(handler, SOCN_WALL_HEADER, GetPlayerNameByGuid(targetGuid), g_wallPageSize);

            QueryResult result = CharacterDatabase.Query(
                "SELECT p.`id`, p.`text`, p.`repostOf`,"
                " IFNULL(o.`authorGuid`, 0) FROM `socn_posts` p"
                " LEFT JOIN `socn_posts` o ON o.`id` = p.`repostOf`"
                " WHERE p.`authorGuid` = {} ORDER BY p.`id` DESC LIMIT {}",
                targetGuid.GetCounter(), g_wallPageSize);

            if (!result)
            {
                SendSocnMessage(handler, SOCN_WALL_EMPTY);
                return true;
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

            return true;
        }

        static bool HandleSocnDeleteCommand(ChatHandler* handler, uint32 postId)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            QueryResult result = CharacterDatabase.Query(
                "SELECT `authorGuid` FROM `socn_posts` WHERE `id` = {}", postId);
            if (!result)
            {
                SendSocnMessage(handler, SOCN_POST_NOT_FOUND, postId);
                return false;
            }

            if (result->Fetch()[0].Get<uint32>() != player->GetGUID().GetCounter())
            {
                SendSocnMessage(handler, SOCN_POST_NOT_YOURS);
                return false;
            }

            CharacterDatabase.Execute("DELETE FROM `socn_posts` WHERE `id` = {}", postId);
            CharacterDatabase.Execute("DELETE FROM `socn_post_likes` WHERE `postId` = {}", postId);
            CharacterDatabase.Execute("DELETE FROM `socn_comments` WHERE `postId` = {}", postId);
            SendSocnMessage(handler, SOCN_POST_DELETED, postId);
            return true;
        }

        static bool HandleSocnDeleteAnyCommand(ChatHandler* handler, uint32 postId)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            QueryResult result = CharacterDatabase.Query(
                "SELECT `authorGuid` FROM `socn_posts` WHERE `id` = {}", postId);
            if (!result)
            {
                SendSocnMessage(handler, SOCN_POST_NOT_FOUND, postId);
                return false;
            }

            CharacterDatabase.Execute("DELETE FROM `socn_posts` WHERE `id` = {}", postId);
            CharacterDatabase.Execute("DELETE FROM `socn_post_likes` WHERE `postId` = {}", postId);
            CharacterDatabase.Execute("DELETE FROM `socn_comments` WHERE `postId` = {}", postId);
            SendSocnMessage(handler, SOCN_POST_DELETED, postId);
            return true;
        }

        static bool HandleSocnLikeCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            if (targetGuid == player->GetGUID())
            {
                SendSocnMessage(handler, SOCN_LIKE_YOURSELF);
                return false;
            }

            QueryResult lastPost = CharacterDatabase.Query(
                "SELECT `id` FROM `socn_posts` WHERE `authorGuid` = {} ORDER BY `id` DESC LIMIT 1",
                targetGuid.GetCounter());
            if (!lastPost)
            {
                SendSocnMessage(handler, SOCN_POST_NOT_FOUND, 0);
                return false;
            }

            uint32 postId = lastPost->Fetch()[0].Get<uint32>();

            if (QueryResult liked = CharacterDatabase.Query(
                "SELECT 1 FROM `socn_post_likes` WHERE `postId` = {} AND `voterGuid` = {}",
                postId, player->GetGUID().GetCounter()))
            {
                SendSocnMessage(handler, SOCN_POST_ALREADY_LIKED);
                return false;
            }

            CharacterDatabase.Execute(
                "INSERT INTO `socn_post_likes` (`postId`, `voterGuid`) VALUES ({}, {})",
                postId, player->GetGUID().GetCounter());

            SendSocnMessage(handler, SOCN_POST_LIKED, target.GetName());
            return true;
        }

        static bool HandleSocnFollowCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            if (targetGuid == player->GetGUID())
            {
                SendSocnMessage(handler, SOCN_FOLLOW_SELF);
                return false;
            }

            if (QueryResult already = CharacterDatabase.Query(
                "SELECT 1 FROM `socn_follows` WHERE `followerGuid` = {} AND `targetGuid` = {}",
                player->GetGUID().GetCounter(), targetGuid.GetCounter()))
            {
                SendSocnMessage(handler, SOCN_FOLLOW_ALREADY, target.GetName());
                return false;
            }

            CharacterDatabase.Execute(
                "INSERT INTO `socn_follows` (`followerGuid`, `targetGuid`) VALUES ({}, {})",
                player->GetGUID().GetCounter(), targetGuid.GetCounter());

            SendSocnMessage(handler, SOCN_FOLLOW_SET, target.GetName());
            return true;
        }

        static bool HandleSocnUnfollowCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            QueryResult existing = CharacterDatabase.Query(
                "SELECT 1 FROM `socn_follows` WHERE `followerGuid` = {} AND `targetGuid` = {}",
                player->GetGUID().GetCounter(), targetGuid.GetCounter());
            if (!existing)
            {
                SendSocnMessage(handler, SOCN_FOLLOW_NOT_YOURS, target.GetName());
                return false;
            }

            CharacterDatabase.Execute(
                "DELETE FROM `socn_follows` WHERE `followerGuid` = {} AND `targetGuid` = {}",
                player->GetGUID().GetCounter(), targetGuid.GetCounter());

            SendSocnMessage(handler, SOCN_FOLLOW_REMOVED, target.GetName());
            return true;
        }

        static bool HandleSocnTopCommand(ChatHandler* handler)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            SendSocnMessage(handler, SOCN_TOP_HEADER, g_topSize);

            QueryResult result = CharacterDatabase.Query(
                "SELECT p.`authorGuid`, COUNT(*) AS `likes` FROM `socn_post_likes` l"
                " JOIN `socn_posts` p ON p.`id` = l.`postId`"
                " GROUP BY p.`authorGuid` ORDER BY `likes` DESC LIMIT {}", g_topSize);

            if (!result)
            {
                SendSocnMessage(handler, SOCN_TOP_EMPTY);
                return true;
            }

            uint32 rank = 0;
            do
            {
                Field* fields = result->Fetch();
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(fields[0].Get<uint32>());
                std::string name = GetPlayerNameByGuid(guid);

                // Mark friends of the viewer
                if (QueryResult isFriend = CharacterDatabase.Query(
                    "SELECT 1 FROM `socn_friends` WHERE `ownerGuid` = {} AND `friendGuid` = {}",
                    player->GetGUID().GetCounter(), guid.GetCounter()))
                {
                    SendSocnMessage(handler, SOCN_TOP_FRIEND_MARK, ++rank, name, fields[1].Get<uint32>());
                }
                else
                    SendSocnMessage(handler, SOCN_TOP_ENTRY, ++rank, name, fields[1].Get<uint32>());
            } while (result->NextRow());

            return true;
        }

        static bool HandleSocnMsgCommand(ChatHandler* handler, PlayerIdentifier target, Tail text)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            if (targetGuid == player->GetGUID())
            {
                SendSocnMessage(handler, SOCN_MSG_SELF);
                return false;
            }

            return SendMessageTo(player, targetGuid, target.GetName(), std::string{ text }, handler);
        }

        static bool HandleSocnInboxCommand(ChatHandler* handler)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            QueryResult result = CharacterDatabase.Query(
                "SELECT `id`, `senderGuid`, `text` FROM `socn_messages`"
                " WHERE `receiverGuid` = {} ORDER BY `id` DESC LIMIT {}",
                player->GetGUID().GetCounter(), g_inboxPageSize);

            if (!result)
            {
                SendSocnMessage(handler, SOCN_INBOX_EMPTY);
                return true;
            }

            do
            {
                Field* fields = result->Fetch();
                ObjectGuid senderGuid = ObjectGuid::Create<HighGuid::Player>(fields[1].Get<uint32>());
                SendSocnMessage(handler, SOCN_INBOX_ENTRY,
                    fields[0].Get<uint32>(), GetPlayerNameByGuid(senderGuid), fields[2].Get<std::string>());
            } while (result->NextRow());

            return true;
        }

        static bool HandleSocnMsgDelCommand(ChatHandler* handler, uint32 msgId)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            QueryResult result = CharacterDatabase.Query(
                "SELECT `receiverGuid` FROM `socn_messages` WHERE `id` = {}", msgId);
            if (!result || result->Fetch()[0].Get<uint32>() != player->GetGUID().GetCounter())
            {
                SendSocnMessage(handler, SOCN_MSG_NOT_FOUND, msgId);
                return false;
            }

            CharacterDatabase.Execute("DELETE FROM `socn_messages` WHERE `id` = {}", msgId);
            SendSocnMessage(handler, SOCN_MSG_DELETED, msgId);
            return true;
        }

        static bool HandleSocnCommentCommand(ChatHandler* handler, uint32 postId, Tail text)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            return AddComment(player, postId, std::string{ text }, handler);
        }

        static bool HandleSocnCommentsCommand(ChatHandler* handler, uint32 postId)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            if (!CharacterDatabase.Query("SELECT 1 FROM `socn_posts` WHERE `id` = {}", postId))
            {
                SendSocnMessage(handler, SOCN_POST_NOT_FOUND, postId);
                return false;
            }

            SendSocnMessage(handler, SOCN_COMMENTS_HEADER, postId);

            QueryResult result = CharacterDatabase.Query(
                "SELECT `authorGuid`, `text` FROM `socn_comments` WHERE `postId` = {} ORDER BY `id` ASC",
                postId);

            if (!result)
            {
                SendSocnMessage(handler, SOCN_COMMENTS_EMPTY);
                return true;
            }

            do
            {
                Field* fields = result->Fetch();
                ObjectGuid authorGuid = ObjectGuid::Create<HighGuid::Player>(fields[0].Get<uint32>());
                SendSocnMessage(handler, SOCN_COMMENT_ENTRY, GetPlayerNameByGuid(authorGuid), fields[1].Get<std::string>());
            } while (result->NextRow());

            return true;
        }

        static bool HandleSocnFriendCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            return SendFriendRequest(player, targetGuid, target.GetName(), handler);
        }

        static bool HandleSocnFriendAcceptCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            return AcceptFriendRequest(player, targetGuid, target.GetName(), handler);
        }

        static bool HandleSocnFriendDeclineCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            return DeclineFriendRequest(player, targetGuid, target.GetName(), handler);
        }

        static bool HandleSocnFriendsCommand(ChatHandler* handler)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            QueryResult result = CharacterDatabase.Query(
                "SELECT `friendGuid` FROM `socn_friends` WHERE `ownerGuid` = {} ORDER BY `friendGuid`",
                player->GetGUID().GetCounter());

            if (!result)
            {
                SendSocnMessage(handler, SOCN_FRIENDS_EMPTY);
                return true;
            }

            std::vector<std::string> names;
            do
            {
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(result->Fetch()[0].Get<uint32>());
                names.push_back(GetPlayerNameByGuid(guid));
            } while (result->NextRow());

            SendSocnMessage(handler, SOCN_FRIENDS_HEADER, names.size());
            for (std::string const& name : names)
                handler->SendSysMessage(name);

            return true;
        }

        static bool HandleSocnRequestsCommand(ChatHandler* handler)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            QueryResult result = CharacterDatabase.Query(
                "SELECT `fromGuid` FROM `socn_friend_requests` WHERE `toGuid` = {} ORDER BY `createdAt` ASC",
                player->GetGUID().GetCounter());

            if (!result)
            {
                SendSocnMessage(handler, SOCN_REQUESTS_EMPTY);
                return true;
            }

            std::vector<std::string> names;
            do
            {
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(result->Fetch()[0].Get<uint32>());
                names.push_back(GetPlayerNameByGuid(guid));
            } while (result->NextRow());

            SendSocnMessage(handler, SOCN_REQUESTS_HEADER, names.size());
            for (std::string const& name : names)
                handler->SendSysMessage(name);

            return true;
        }

        static bool HandleSocnFriendDelCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            return RemoveFriend(player, targetGuid, target.GetName(), handler);
        }

        static bool HandleSocnEnemyCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            return AddEnemy(player, targetGuid, target.GetName(), handler);
        }

        static bool HandleSocnEnemiesCommand(ChatHandler* handler)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            QueryResult result = CharacterDatabase.Query(
                "SELECT `enemyGuid` FROM `socn_enemies` WHERE `ownerGuid` = {} ORDER BY `enemyGuid`",
                player->GetGUID().GetCounter());

            if (!result)
            {
                SendSocnMessage(handler, SOCN_ENEMIES_EMPTY);
                return true;
            }

            std::vector<std::string> names;
            do
            {
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(result->Fetch()[0].Get<uint32>());
                names.push_back(GetPlayerNameByGuid(guid));
            } while (result->NextRow());

            SendSocnMessage(handler, SOCN_ENEMIES_HEADER, names.size());
            for (std::string const& name : names)
                handler->SendSysMessage(name);

            return true;
        }

        static bool HandleSocnEnemyDelCommand(ChatHandler* handler, PlayerIdentifier target)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            ObjectGuid targetGuid = FindCharacterGuidByName(std::string(target.GetName()));
            if (targetGuid.IsEmpty())
            {
                handler->SendErrorMessage("Character '{}' not found.", target.GetName());
                return false;
            }

            return RemoveEnemy(player, targetGuid, target.GetName(), handler);
        }

        static bool HandleSocnGuildCommand(ChatHandler* handler)
        {
            Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
            if (!player)
                return false;

            uint32 guildId = player->GetGuildId();
            if (!guildId)
            {
                SendSocnMessage(handler, SOCN_GUILD_NONE);
                return true;
            }

            std::string guildName;
            ObjectGuid masterGuid = ObjectGuid::Empty;
            if (QueryResult result = CharacterDatabase.Query(
                "SELECT `name`, `leaderguid` FROM `guild` WHERE `guildid` = {}", guildId))
            {
                guildName = result->Fetch()[0].Get<std::string>();
                masterGuid = ObjectGuid::Create<HighGuid::Player>(result->Fetch()[1].Get<uint32>());
            }

            SendSocnMessage(handler, SOCN_GUILD_HEADER, guildName);
            SendSocnMessage(handler, SOCN_GUILD_MASTER, GetPlayerNameByGuid(masterGuid));

            QueryResult result = CharacterDatabase.Query(
                "SELECT `guid` FROM `guild_member` WHERE `guildid` = {} ORDER BY `guid` LIMIT {}",
                guildId, g_guildListSize);

            uint32 memberCount = 0;
            uint32 onlineCount = 0;
            uint32 listRank = 0;
            if (QueryResult total = CharacterDatabase.Query(
                "SELECT COUNT(*) FROM `guild_member` WHERE `guildid` = {}", guildId))
                memberCount = total->Fetch()[0].Get<uint32>();

            SendSocnMessage(handler, SOCN_GUILD_MEMBERS, memberCount);

            if (result)
            {
                do
                {
                    ObjectGuid memberGuid = ObjectGuid::Create<HighGuid::Player>(result->Fetch()[0].Get<uint32>());
                    if (ObjectAccessor::FindConnectedPlayer(memberGuid))
                    {
                        ++onlineCount;
                        SendSocnMessage(handler, SOCN_GUILD_ONLINE_ENTRY, ++listRank, GetPlayerNameByGuid(memberGuid));
                    }
                    else
                        SendSocnMessage(handler, SOCN_GUILD_OFFLINE_ENTRY, ++listRank, GetPlayerNameByGuid(memberGuid));
                } while (result->NextRow());
            }

            SendSocnMessage(handler, SOCN_PROFILE_ENEMIES, onlineCount);
            return true;
        }
    };

    class SocnWorldScript : public WorldScript
    {
    public:
        SocnWorldScript() : WorldScript("SocnWorldScript") { }

        void OnAfterConfigLoad(bool /*reload*/) override
        {
            g_maxPosts = sConfigMgr->GetOption<uint32>("SocialNetwork.MaxPosts", 10);
            g_maxStatusLen = sConfigMgr->GetOption<uint32>("SocialNetwork.MaxStatusLength", 60);
            g_maxPostLen = sConfigMgr->GetOption<uint32>("SocialNetwork.MaxPostLength", 255);
            g_wallPageSize = sConfigMgr->GetOption<uint32>("SocialNetwork.WallPageSize", 5);
            g_topSize = sConfigMgr->GetOption<uint32>("SocialNetwork.TopSize", 10);
            g_inboxPageSize = sConfigMgr->GetOption<uint32>("SocialNetwork.InboxPageSize", 10);
            g_maxMsgLen = sConfigMgr->GetOption<uint32>("SocialNetwork.MaxMessageLength", 255);
            g_maxComments = sConfigMgr->GetOption<uint32>("SocialNetwork.MaxComments", 20);
            g_maxFriends = sConfigMgr->GetOption<uint32>("SocialNetwork.MaxFriends", 50);
            g_maxEnemies = sConfigMgr->GetOption<uint32>("SocialNetwork.MaxEnemies", 10);
            g_guildListSize = sConfigMgr->GetOption<uint32>("SocialNetwork.GuildListSize", 10);
        }
    };
}

void Add_mod_social_networkScripts()
{
    new SocnCommandScript();
    new SocnWorldScript();
}
