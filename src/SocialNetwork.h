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

#ifndef MOD_SOCIAL_NETWORK_H
#define MOD_SOCIAL_NETWORK_H

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "StringFormat.h"
#include "Util.h"
#include <algorithm>
#include <string>

namespace Socn
{
    // Matches the `module` column in module_string / module_string_locale
    constexpr char const* MODULE_NAME = "mod-social-network";

    // Custom RBAC permissions (custom range 1100+), seeded by data/sql/db-characters/social_network.sql
    constexpr uint32 PERM_COMMAND = 1100;
    constexpr uint32 PERM_COMMAND_DELETE_ANY = 1101;

    // Localized strings, seeded by data/sql/db-characters/social_network.sql
    enum StringId : uint32
    {
        SOCN_PROFILE_HEADER = 1,
        SOCN_PROFILE_GUILD_NONE = 2,
        SOCN_PROFILE_GUILD = 3,
        SOCN_PROFILE_STATUS_NONE = 4,
        SOCN_PROFILE_STATUS = 5,
        SOCN_PROFILE_LIKES = 6,
        SOCN_PROFILE_FOLLOWERS = 7,
        SOCN_PROFILE_FOLLOWING = 8,
        SOCN_STATUS_SET = 9,
        SOCN_POST_SET = 10,
        SOCN_POST_EMPTY = 11,
        SOCN_WALL_HEADER = 12,
        SOCN_WALL_EMPTY = 13,
        SOCN_WALL_ENTRY = 14,
        SOCN_POST_DELETED = 15,
        SOCN_POST_NOT_FOUND = 16,
        SOCN_POST_NOT_YOURS = 17,
        SOCN_POST_LIKED = 18,
        SOCN_POST_ALREADY_LIKED = 19,
        SOCN_LIKE_YOURSELF = 20,
        SOCN_FOLLOW_SET = 21,
        SOCN_FOLLOW_REMOVED = 22,
        SOCN_FOLLOW_NOT_YOURS = 23,
        SOCN_FOLLOW_ALREADY = 24,
        SOCN_FOLLOW_SELF = 25,
        SOCN_TOP_HEADER = 26,
        SOCN_TOP_EMPTY = 27,
        SOCN_TOP_ENTRY = 28,
        SOCN_LIMIT_REACHED = 29,
        SOCN_TEXT_TOO_LONG = 30,
        SOCN_FOLLOW_NOTIFY = 31,
        SOCN_GOSSIP_TITLE = 32,
        SOCN_GOSSIP_PROFILE = 33,
        SOCN_GOSSIP_WALL = 34,
        SOCN_GOSSIP_POST = 35,
        SOCN_GOSSIP_INBOX = 36,
        SOCN_GOSSIP_TOP = 37,
        SOCN_GOSSIP_CLOSE = 38,
        SOCN_MSG_SENT = 39,
        SOCN_MSG_SELF = 40,
        SOCN_INBOX_EMPTY = 41,
        SOCN_INBOX_ENTRY = 42,
        SOCN_MSG_DELETED = 43,
        SOCN_COMMENT_SET = 44,
        SOCN_COMMENTS_HEADER = 45,
        SOCN_COMMENT_ENTRY = 46,
        SOCN_COMMENTS_EMPTY = 47,
        SOCN_PROFILE_LEVEL = 48,
        SOCN_PROFILE_PLAYTIME = 49,
        SOCN_MSG_NOT_FOUND = 50,
        SOCN_GOSSIP_HELP = 51,
        SOCN_FRIEND_REQUEST_SENT = 52,
        SOCN_FRIEND_REQUEST_RECEIVED = 53,
        SOCN_FRIEND_SET = 54,
        SOCN_FRIEND_ALREADY = 55,
        SOCN_FRIEND_REQUEST_EXISTS = 56,
        SOCN_FRIEND_REQUEST_NOT_FOUND = 57,
        SOCN_FRIEND_REMOVED = 58,
        SOCN_FRIEND_NOT_YOURS = 59,
        SOCN_FRIEND_SELF = 60,
        SOCN_FRIENDS_HEADER = 61,
        SOCN_FRIENDS_EMPTY = 62,
        SOCN_ENEMY_SET = 63,
        SOCN_ENEMY_REMOVED = 64,
        SOCN_ENEMY_SELF = 65,
        SOCN_ENEMY_NOT_YOURS = 66,
        SOCN_ENEMIES_HEADER = 67,
        SOCN_ENEMIES_EMPTY = 68,
        SOCN_ENEMY_ALREADY = 69,
        SOCN_PROFILE_FRIENDS = 70,
        SOCN_PROFILE_ENEMIES = 71,
        SOCN_GUILD_HEADER = 72,
        SOCN_GUILD_MEMBERS = 73,
        SOCN_GUILD_MASTER = 74,
        SOCN_GUILD_ONLINE_ENTRY = 75,
        SOCN_GUILD_OFFLINE_ENTRY = 76,
        SOCN_GUILD_NONE = 77,
        SOCN_FRIEND_LIMIT = 78,
        SOCN_ENEMY_LIMIT = 79,
        SOCN_REQUEST_DECLINED = 80,
        SOCN_ENEMY_IS_FRIEND = 81,
        SOCN_REQUESTS_HEADER = 82,
        SOCN_REQUESTS_EMPTY = 83,
        SOCN_TOP_FRIEND_MARK = 84,
        SOCN_REPOST_DONE = 85,
        SOCN_REPOST_SELF = 86,
        SOCN_REPOST_ALREADY = 87,
        SOCN_WALL_REPOST_ENTRY = 88,
        SOCN_REPOST_NOTIFY = 89
    };

    // Config (only touched from the world thread)
    inline uint32 g_maxPosts = 10;
    inline uint32 g_maxStatusLen = 60;    // in characters (UTF-8 aware)
    inline uint32 g_maxPostLen = 255;     // in characters (UTF-8 aware)
    inline uint32 g_wallPageSize = 5;
    inline uint32 g_topSize = 10;
    inline uint32 g_inboxPageSize = 10;
    inline uint32 g_maxMsgLen = 255;      // in characters (UTF-8 aware)
    inline uint32 g_maxComments = 20;     // per post
    inline uint32 g_maxFriends = 50;
    inline uint32 g_maxEnemies = 10;
    inline uint32 g_guildListSize = 10;

    // English fallbacks, used only when the module_string rows are missing (SQL not applied)
    inline std::string_view GetEnglishFallback(uint32 id)
    {
        switch (id)
        {
            case SOCN_PROFILE_HEADER:
                return "=== Profile: {} ===";
            case SOCN_PROFILE_GUILD_NONE:
                return "Guild: none";
            case SOCN_PROFILE_GUILD:
                return "Guild: {}";
            case SOCN_PROFILE_STATUS_NONE:
                return "Status: not set";
            case SOCN_PROFILE_STATUS:
                return "Status: {}";
            case SOCN_PROFILE_LIKES:
                return "Total likes: {}";
            case SOCN_PROFILE_FOLLOWERS:
                return "Followers: {}";
            case SOCN_PROFILE_FOLLOWING:
                return "Following: {}";
            case SOCN_STATUS_SET:
                return "Status updated.";
            case SOCN_POST_SET:
                return "Post published on your wall.";
            case SOCN_POST_EMPTY:
                return "Message is empty.";
            case SOCN_WALL_HEADER:
                return "=== Wall of {} (last {}) ===";
            case SOCN_WALL_EMPTY:
                return "The wall is empty.";
            case SOCN_WALL_ENTRY:
                return "[{}] {}";
            case SOCN_POST_DELETED:
                return "Post {} deleted.";
            case SOCN_POST_NOT_FOUND:
                return "Post {} not found.";
            case SOCN_POST_NOT_YOURS:
                return "You cannot delete this post.";
            case SOCN_POST_LIKED:
                return "You liked the post of {}.";
            case SOCN_POST_ALREADY_LIKED:
                return "You already liked this post.";
            case SOCN_LIKE_YOURSELF:
                return "You cannot like your own post.";
            case SOCN_FOLLOW_SET:
                return "You are now following {}.";
            case SOCN_FOLLOW_REMOVED:
                return "You stopped following {}.";
            case SOCN_FOLLOW_NOT_YOURS:
                return "You are not following {}.";
            case SOCN_FOLLOW_ALREADY:
                return "You are already following {}.";
            case SOCN_FOLLOW_SELF:
                return "You cannot follow yourself.";
            case SOCN_TOP_HEADER:
                return "=== Top {} by likes ===";
            case SOCN_TOP_EMPTY:
                return "No rated players yet.";
            case SOCN_TOP_ENTRY:
                return "{}. {} — {} likes";
            case SOCN_LIMIT_REACHED:
                return "Post limit reached ({}). Delete an old post first.";
            case SOCN_TEXT_TOO_LONG:
                return "Text is too long (max {} characters).";
            case SOCN_FOLLOW_NOTIFY:
                return "{} posted on the wall: \"{}\"";
            case SOCN_GOSSIP_TITLE:
                return "Social Network";
            case SOCN_GOSSIP_PROFILE:
                return "My profile";
            case SOCN_GOSSIP_WALL:
                return "My wall";
            case SOCN_GOSSIP_POST:
                return "Write a post...";
            case SOCN_GOSSIP_INBOX:
                return "Inbox";
            case SOCN_GOSSIP_TOP:
                return "Top by likes";
            case SOCN_GOSSIP_CLOSE:
                return "Close";
            case SOCN_MSG_SENT:
                return "Message sent to {}.";
            case SOCN_MSG_SELF:
                return "You cannot message yourself.";
            case SOCN_INBOX_EMPTY:
                return "Inbox is empty.";
            case SOCN_INBOX_ENTRY:
                return "[{}] From {}: {}";
            case SOCN_MSG_DELETED:
                return "Message {} deleted.";
            case SOCN_COMMENT_SET:
                return "Comment added.";
            case SOCN_COMMENTS_HEADER:
                return "=== Comments for post {} ===";
            case SOCN_COMMENT_ENTRY:
                return "{}: {}";
            case SOCN_COMMENTS_EMPTY:
                return "No comments yet.";
            case SOCN_PROFILE_LEVEL:
                return "Level: {}";
            case SOCN_PROFILE_PLAYTIME:
                return "Play time: {}";
            case SOCN_MSG_NOT_FOUND:
                return "Message {} not found.";
            case SOCN_GOSSIP_HELP:
                return "Hint: .social msg <player> <text> - private message, .social comment <postId> <text> - comment.";
            case SOCN_FRIEND_REQUEST_SENT:
                return "Friend request sent to {}.";
            case SOCN_FRIEND_REQUEST_RECEIVED:
                return "{} sent you a friend request. Use .social friendaccept {}";
            case SOCN_FRIEND_SET:
                return "{} is now your friend.";
            case SOCN_FRIEND_ALREADY:
                return "{} is already your friend.";
            case SOCN_FRIEND_REQUEST_EXISTS:
                return "A friend request to {} is already pending.";
            case SOCN_FRIEND_REQUEST_NOT_FOUND:
                return "No pending friend request from {}.";
            case SOCN_FRIEND_REMOVED:
                return "{} removed from friends.";
            case SOCN_FRIEND_NOT_YOURS:
                return "{} is not your friend.";
            case SOCN_FRIEND_SELF:
                return "You cannot add yourself.";
            case SOCN_FRIENDS_HEADER:
                return "=== Friends ({}) ===";
            case SOCN_FRIENDS_EMPTY:
                return "No friends yet. Send requests with .social friend <player>.";
            case SOCN_ENEMY_SET:
                return "{} added to enemies.";
            case SOCN_ENEMY_REMOVED:
                return "{} removed from enemies.";
            case SOCN_ENEMY_SELF:
                return "You cannot add yourself.";
            case SOCN_ENEMY_NOT_YOURS:
                return "{} is not in your enemy list.";
            case SOCN_ENEMIES_HEADER:
                return "=== Enemies ({}) ===";
            case SOCN_ENEMIES_EMPTY:
                return "Enemy list is empty.";
            case SOCN_ENEMY_ALREADY:
                return "{} is already in your enemy list.";
            case SOCN_PROFILE_FRIENDS:
                return "Friends: {} | Enemies: {}";
            case SOCN_PROFILE_ENEMIES:
                return "Guild online: {}";
            case SOCN_GUILD_HEADER:
                return "=== Guild {} ===";
            case SOCN_GUILD_MEMBERS:
                return "Members: {}";
            case SOCN_GUILD_MASTER:
                return "Guild master: {}";
            case SOCN_GUILD_ONLINE_ENTRY:
                return "[{}] {} (online)";
            case SOCN_GUILD_OFFLINE_ENTRY:
                return "[{}] {}";
            case SOCN_GUILD_NONE:
                return "You are not in a guild.";
            case SOCN_FRIEND_LIMIT:
                return "Friend list is full ({}).";
            case SOCN_ENEMY_LIMIT:
                return "Enemy list is full ({}).";
            case SOCN_REQUEST_DECLINED:
                return "Friend request declined.";
            case SOCN_ENEMY_IS_FRIEND:
                return "{} is your friend - remove the friendship first.";
            case SOCN_REQUESTS_HEADER:
                return "=== Incoming friend requests ({}) ===";
            case SOCN_REQUESTS_EMPTY:
                return "No incoming friend requests.";
            case SOCN_TOP_FRIEND_MARK:
                return "{}. [Friend] {} - {} likes";
            case SOCN_REPOST_DONE:
                return "Post {} reposted to your wall.";
            case SOCN_REPOST_SELF:
                return "You cannot repost your own post.";
            case SOCN_REPOST_ALREADY:
                return "You already reposted this post.";
            case SOCN_WALL_REPOST_ENTRY:
                return "[{}] {} (repost of {}'s post)";
            case SOCN_REPOST_NOTIFY:
                return "{} reposted your post: \"{}\"";
            default:
                return "";
        }
    }

    // Returns the string on the receiver's locale (session locale for players,
    // default DBC locale for console); falls back to English if SQL is not applied.
    inline std::string GetSocnString(ChatHandler const* handler, uint32 id)
    {
        if (std::string const* str = handler->GetModuleString(MODULE_NAME, id))
            return *str;
        return std::string(GetEnglishFallback(id));
    }

    // Localized message to the receiver
    template<typename... Args>
    inline void SendSocnMessage(ChatHandler* handler, uint32 id, Args&&... args)
    {
        handler->SendSysMessage(Acore::StringFormat(GetSocnString(handler, id), std::forward<Args>(args)...));
    }

    inline std::string GetPlayerNameByGuid(ObjectGuid guid)
    {
        if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
            return player->GetName();

        if (ObjectGuid::LowType lowGuid = guid.GetCounter())
            if (QueryResult result = CharacterDatabase.Query("SELECT `name` FROM `characters` WHERE `guid` = {}", lowGuid))
                return result->Fetch()[0].Get<std::string>();

        return "Unknown";
    }

    // Escapes a string for direct embedding into a SQL query literal ('{}').
    // Required for all user-provided text: without it a single quote in the
    // input breaks the query and allows SQL injection.
    inline std::string SqlEscape(std::string const& text)
    {
        std::string escaped = text;
        CharacterDatabase.EscapeString(escaped);
        return escaped;
    }

    // Finds a character by name (case-insensitive, UTF-8 aware), returns its guid (Empty if not found)
    inline ObjectGuid FindCharacterGuidByName(std::string const& name)
    {
        if (Player* player = ObjectAccessor::FindPlayerByName(name, false))
            return player->GetGUID();

        // Normalize case through wide chars so Cyrillic/extended names resolve too
        std::wstring wname;
        if (!Utf8toWStr(name, wname))
            return ObjectGuid::Empty;
        wstrToLower(wname);

        std::string lowName;
        if (!WStrToUtf8(wname, lowName))
            return ObjectGuid::Empty;

        QueryResult result = CharacterDatabase.Query("SELECT `guid` FROM `characters` WHERE LOWER(`name`) = '{}'", SqlEscape(lowName));
        if (!result)
            return ObjectGuid::Empty;

        return ObjectGuid::Create<HighGuid::Player>(result->Fetch()[0].Get<uint32>());
    }

    // Number of characters (not bytes) in a UTF-8 string
    inline std::size_t Utf8CharLength(std::string const& text)
    {
        std::size_t count = 0;
        for (unsigned char c : text)
            if ((c & 0xC0) != 0x80) // count only non-continuation bytes
                ++count;
        return count;
    }

    // Client (3.3.5) chat renders color codes; strip them from user text to keep
    // content clean and prevent chat-escape tricks.
    inline void SanitizeForClient(std::string& text)
    {
        std::size_t pos;
        while ((pos = text.find('|')) != std::string::npos)
            text.erase(pos, 1);
    }

    // Emoticon -> client texture code mapping. Uses built-in 3.3.5 interface
    // textures rendered via |Tpath:size|t codes (safe after SanitizeForClient,
    // which strips all '|' from user input before this runs).
    struct EmoteIcon
    {
        char const* smiley;
        char const* texture;
    };

    inline constexpr EmoteIcon g_emotes[] =
    {
        // Longest sequences first: ":)" must not match inside ":-)",
        // "<3" inside "</3", ":(" inside ">:(", "zzz" inside "Zzz".
        { "</3",  "Interface\\Icons\\INV_ValentineCard_01" },     // broken heart
        { ">:(",  "Interface\\Icons\\INV_Misc_Gem_01" },          // angry
        { ":-)",  "Interface\\Common\\ReputationStar" },
        { ":-D",  "Interface\\Icons\\INV_Misc_Gem_Variety_01" },  // big grin
        { ";-)",  "Interface\\Icons\\INV_Misc_Eye_01" },          // wink
        { ":-P",  "Interface\\Icons\\INV_Misc_Food_59" },         // tongue
        { ":-(",  "Interface\\Icons\\Spell_Shadow_Charm" },       // sad
        { "\\o/", "Interface\\Icons\\INV_Misc_Toy_05" },          // hooray
        { "Zzz",  "Interface\\Icons\\INV_Misc_Bed_01" },          // sleepy
        { "o_O",  "Interface\\Icons\\INV_Misc_Eye_02" },          // surprised
        { "O_o",  "Interface\\Icons\\INV_Misc_Eye_02" },
        { ":)",   "Interface\\Common\\ReputationStar" },          // star smile
        { ":D",   "Interface\\Icons\\INV_Misc_Gem_Variety_01" },
        { ";)",   "Interface\\Icons\\INV_Misc_Eye_01" },
        { ":P",   "Interface\\Icons\\INV_Misc_Food_59" },
        { ":(",   "Interface\\Icons\\Spell_Shadow_Charm" },
        { "<3",   "Interface\\Icons\\INV_ValentineRoseBouquet01" }, // heart (roses)
        { ":O",   "Interface\\Icons\\INV_Misc_Gem_02" },          // shock
        { "zzz",  "Interface\\Icons\\INV_Misc_Bed_01" },
        { ":*",   "Interface\\Icons\\INV_Misc_Gem_03" },          // kiss
    };

    inline constexpr std::size_t g_emotesCount = std::size(g_emotes);

    // Replaces text emoticons with client icon codes; returns false if nothing changed
    inline bool ApplyEmotes(std::string& text)
    {
        bool changed = false;
        for (std::size_t i = 0; i < g_emotesCount; ++i)
        {
            std::string const smiley = g_emotes[i].smiley;
            std::string code = "|T" + std::string(g_emotes[i].texture) + ":14|t";

            std::size_t pos;
            while ((pos = text.find(smiley)) != std::string::npos)
            {
                text.replace(pos, smiley.length(), code);
                changed = true;
            }
        }
        return changed;
    }

    // Byte limit for text columns (varchar(255) in the SQL schema); a truncated
    // multibyte UTF-8 tail is also invalid SQL data, so bytes are checked too.
    constexpr std::size_t MAX_TEXT_BYTES = 255;

    // Validates user text: sanitizes, applies emoticons, checks emptiness and
    // character length (UTF-8 aware). Runs before storage so walls, comments
    // and messages already contain rendered icon codes.
    inline bool ValidateText(ChatHandler* handler, std::string& text, uint32 maxLen)
    {
        SanitizeForClient(text);

        if (text.empty())
        {
            SendSocnMessage(handler, SOCN_POST_EMPTY);
            return false;
        }

        if (Utf8CharLength(text) > maxLen)
        {
            SendSocnMessage(handler, SOCN_TEXT_TOO_LONG, maxLen);
            return false;
        }

        // Emoticons expand into |T...|t codes; expansion happens after the length
        // check so the player is limited by typed characters, not rendered bytes.
        ApplyEmotes(text);

        // Rendered codes inflate the byte size well beyond the typed characters;
        // the check keeps the text within the column byte limit (SQL strict mode
        // would reject the insert otherwise).
        if (text.size() > MAX_TEXT_BYTES)
        {
            SendSocnMessage(handler, SOCN_TEXT_TOO_LONG, maxLen);
            return false;
        }

        return true;
    }

    // Publishes a post on the player's wall and notifies online followers
    inline bool PostToWall(Player* player, std::string post, ChatHandler* handler)
    {
        if (!ValidateText(handler, post, g_maxPostLen))
            return false;

        if (QueryResult result = CharacterDatabase.Query(
            "SELECT COUNT(*) FROM `socn_posts` WHERE `authorGuid` = {}", player->GetGUID().GetCounter()))
        {
            if (result->Fetch()[0].Get<uint32>() >= g_maxPosts)
            {
                SendSocnMessage(handler, SOCN_LIMIT_REACHED, g_maxPosts);
                return false;
            }
        }

        CharacterDatabase.Execute(
            "INSERT INTO `socn_posts` (`authorGuid`, `text`) VALUES ({}, '{}')",
            player->GetGUID().GetCounter(), SqlEscape(post));

        if (QueryResult followers = CharacterDatabase.Query(
            "SELECT `followerGuid` FROM `socn_follows` WHERE `targetGuid` = {}",
            player->GetGUID().GetCounter()))
        {
            do
            {
                ObjectGuid followerGuid = ObjectGuid::Create<HighGuid::Player>(followers->Fetch()[0].Get<uint32>());
                if (Player* follower = ObjectAccessor::FindConnectedPlayer(followerGuid))
                {
                    ChatHandler followerHandler(follower->GetSession());
                    SendSocnMessage(&followerHandler, SOCN_FOLLOW_NOTIFY, player->GetName(), post);
                }
            } while (followers->NextRow());
        }

        SendSocnMessage(handler, SOCN_POST_SET);
        return true;
    }

    // Reposts another player's post onto the player's wall. Stores a snapshot
    // of the original text, so the wall survives deletion of the original.
    inline bool RepostToWall(Player* player, uint32 postId, ChatHandler* handler)
    {
        QueryResult original = CharacterDatabase.Query(
            "SELECT `authorGuid`, `text` FROM `socn_posts` WHERE `id` = {}", postId);
        if (!original)
        {
            SendSocnMessage(handler, SOCN_POST_NOT_FOUND, postId);
            return false;
        }

        Field* fields = original->Fetch();
        uint32 sourceGuid = fields[0].Get<uint32>();
        ObjectGuid source = ObjectGuid::Create<HighGuid::Player>(sourceGuid);

        if (sourceGuid == player->GetGUID().GetCounter())
        {
            SendSocnMessage(handler, SOCN_REPOST_SELF);
            return false;
        }

        if (QueryResult already = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_posts` WHERE `authorGuid` = {} AND `repostOf` = {}",
            player->GetGUID().GetCounter(), postId))
        {
            SendSocnMessage(handler, SOCN_REPOST_ALREADY);
            return false;
        }

        if (QueryResult result = CharacterDatabase.Query(
            "SELECT COUNT(*) FROM `socn_posts` WHERE `authorGuid` = {}", player->GetGUID().GetCounter()))
        {
            if (result->Fetch()[0].Get<uint32>() >= g_maxPosts)
            {
                SendSocnMessage(handler, SOCN_LIMIT_REACHED, g_maxPosts);
                return false;
            }
        }

        std::string text = fields[1].Get<std::string>();

        CharacterDatabase.Execute(
            "INSERT INTO `socn_posts` (`authorGuid`, `text`, `repostOf`) VALUES ({}, '{}', {})",
            player->GetGUID().GetCounter(), SqlEscape(text), postId);

        // Notify the original author if online
        if (Player* sourcePlayer = ObjectAccessor::FindConnectedPlayer(source))
        {
            ChatHandler sourceHandler(sourcePlayer->GetSession());
            SendSocnMessage(&sourceHandler, SOCN_REPOST_NOTIFY, player->GetName(), text);
        }

        SendSocnMessage(handler, SOCN_REPOST_DONE, postId);
        return true;
    }

    // Sends a private message and delivers it instantly if the receiver is online
    inline bool SendMessageTo(Player* player, ObjectGuid targetGuid, std::string const& targetName,
        std::string text, ChatHandler* handler)
    {
        if (!ValidateText(handler, text, g_maxMsgLen))
            return false;

        CharacterDatabase.Execute(
            "INSERT INTO `socn_messages` (`senderGuid`, `receiverGuid`, `text`) VALUES ({}, {}, '{}')",
            player->GetGUID().GetCounter(), targetGuid.GetCounter(), SqlEscape(text));

        if (Player* receiver = ObjectAccessor::FindConnectedPlayer(targetGuid))
        {
            ChatHandler receiverHandler(receiver->GetSession());
            SendSocnMessage(&receiverHandler, SOCN_INBOX_ENTRY, 0, player->GetName(), text);
        }

        SendSocnMessage(handler, SOCN_MSG_SENT, targetName);
        return true;
    }

    // Adds a comment to a post
    inline bool AddComment(Player* player, uint32 postId, std::string text, ChatHandler* handler)
    {
        if (!ValidateText(handler, text, g_maxMsgLen))
            return false;

        if (!CharacterDatabase.Query("SELECT 1 FROM `socn_posts` WHERE `id` = {}", postId))
        {
            SendSocnMessage(handler, SOCN_POST_NOT_FOUND, postId);
            return false;
        }

        if (QueryResult result = CharacterDatabase.Query(
            "SELECT COUNT(*) FROM `socn_comments` WHERE `postId` = {}", postId))
        {
            if (result->Fetch()[0].Get<uint32>() >= g_maxComments)
            {
                SendSocnMessage(handler, SOCN_LIMIT_REACHED, g_maxComments);
                return false;
            }
        }

        CharacterDatabase.Execute(
            "INSERT INTO `socn_comments` (`postId`, `authorGuid`, `text`) VALUES ({}, {}, '{}')",
            postId, player->GetGUID().GetCounter(), SqlEscape(text));

        SendSocnMessage(handler, SOCN_COMMENT_SET);
        return true;
    }

    // ==================== Friends ====================

    // Sends a mutual friendship request; when accepted, both sides become friends
    inline bool SendFriendRequest(Player* player, ObjectGuid targetGuid, std::string const& targetName,
        ChatHandler* handler)
    {
        if (targetGuid == player->GetGUID())
        {
            SendSocnMessage(handler, SOCN_FRIEND_SELF);
            return false;
        }

        if (QueryResult existing = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_friends` WHERE (`ownerGuid` = {} AND `friendGuid` = {})"
            " OR (`ownerGuid` = {} AND `friendGuid` = {})",
            player->GetGUID().GetCounter(), targetGuid.GetCounter(),
            targetGuid.GetCounter(), player->GetGUID().GetCounter()))
        {
            SendSocnMessage(handler, SOCN_FRIEND_ALREADY, targetName);
            return false;
        }

        if (QueryResult already = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_friend_requests` WHERE `fromGuid` = {} AND `toGuid` = {}",
            player->GetGUID().GetCounter(), targetGuid.GetCounter()))
        {
            SendSocnMessage(handler, SOCN_FRIEND_REQUEST_EXISTS, targetName);
            return false;
        }

        if (QueryResult count = CharacterDatabase.Query(
            "SELECT COUNT(*) FROM `socn_friends` WHERE `ownerGuid` = {}", player->GetGUID().GetCounter()))
        {
            if (count->Fetch()[0].Get<uint32>() >= g_maxFriends)
            {
                SendSocnMessage(handler, SOCN_FRIEND_LIMIT, g_maxFriends);
                return false;
            }
        }

        CharacterDatabase.Execute(
            "REPLACE INTO `socn_friend_requests` (`fromGuid`, `toGuid`) VALUES ({}, {})",
            player->GetGUID().GetCounter(), targetGuid.GetCounter());

        // Instant delivery to an online receiver
        if (Player* receiver = ObjectAccessor::FindConnectedPlayer(targetGuid))
        {
            ChatHandler receiverHandler(receiver->GetSession());
            SendSocnMessage(&receiverHandler, SOCN_FRIEND_REQUEST_RECEIVED,
                player->GetName(), player->GetName());
        }

        SendSocnMessage(handler, SOCN_FRIEND_REQUEST_SENT, targetName);
        return true;
    }

    inline bool AcceptFriendRequest(Player* player, ObjectGuid targetGuid, std::string const& targetName,
        ChatHandler* handler)
    {
        QueryResult request = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_friend_requests` WHERE `fromGuid` = {} AND `toGuid` = {}",
            targetGuid.GetCounter(), player->GetGUID().GetCounter());
        if (!request)
        {
            SendSocnMessage(handler, SOCN_FRIEND_REQUEST_NOT_FOUND, targetName);
            return false;
        }

        CharacterDatabase.Execute(
            "DELETE FROM `socn_friend_requests` WHERE `fromGuid` = {} AND `toGuid` = {}",
            targetGuid.GetCounter(), player->GetGUID().GetCounter());

        // Mutual friendship: two rows
        CharacterDatabase.Execute(
            "INSERT IGNORE INTO `socn_friends` (`ownerGuid`, `friendGuid`) VALUES ({}, {}), ({}, {})",
            player->GetGUID().GetCounter(), targetGuid.GetCounter(),
            targetGuid.GetCounter(), player->GetGUID().GetCounter());

        SendSocnMessage(handler, SOCN_FRIEND_SET, targetName);

        if (Player* requester = ObjectAccessor::FindConnectedPlayer(targetGuid))
        {
            ChatHandler requesterHandler(requester->GetSession());
            SendSocnMessage(&requesterHandler, SOCN_FRIEND_SET, player->GetName());
        }
        return true;
    }

    inline bool DeclineFriendRequest(Player* player, ObjectGuid targetGuid, std::string const& targetName,
        ChatHandler* handler)
    {
        QueryResult request = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_friend_requests` WHERE `fromGuid` = {} AND `toGuid` = {}",
            targetGuid.GetCounter(), player->GetGUID().GetCounter());
        if (!request)
        {
            SendSocnMessage(handler, SOCN_FRIEND_REQUEST_NOT_FOUND, targetName);
            return false;
        }

        CharacterDatabase.Execute(
            "DELETE FROM `socn_friend_requests` WHERE `fromGuid` = {} AND `toGuid` = {}",
            targetGuid.GetCounter(), player->GetGUID().GetCounter());

        SendSocnMessage(handler, SOCN_REQUEST_DECLINED);
        return true;
    }

    // Removes friendship on both sides
    inline bool RemoveFriend(Player* player, ObjectGuid targetGuid, std::string const& targetName,
        ChatHandler* handler)
    {
        QueryResult existing = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_friends` WHERE `ownerGuid` = {} AND `friendGuid` = {}",
            player->GetGUID().GetCounter(), targetGuid.GetCounter());
        if (!existing)
        {
            SendSocnMessage(handler, SOCN_FRIEND_NOT_YOURS, targetName);
            return false;
        }

        CharacterDatabase.Execute(
            "DELETE FROM `socn_friends` WHERE (`ownerGuid` = {} AND `friendGuid` = {})"
            " OR (`ownerGuid` = {} AND `friendGuid` = {})",
            player->GetGUID().GetCounter(), targetGuid.GetCounter(),
            targetGuid.GetCounter(), player->GetGUID().GetCounter());

        SendSocnMessage(handler, SOCN_FRIEND_REMOVED, targetName);
        return true;
    }

    // ==================== Enemies ====================

    inline bool AddEnemy(Player* player, ObjectGuid targetGuid, std::string const& targetName,
        ChatHandler* handler)
    {
        if (targetGuid == player->GetGUID())
        {
            SendSocnMessage(handler, SOCN_ENEMY_SELF);
            return false;
        }

        if (QueryResult isFriend = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_friends` WHERE `ownerGuid` = {} AND `friendGuid` = {}",
            player->GetGUID().GetCounter(), targetGuid.GetCounter()))
        {
            SendSocnMessage(handler, SOCN_ENEMY_IS_FRIEND, targetName);
            return false;
        }

        if (QueryResult already = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_enemies` WHERE `ownerGuid` = {} AND `enemyGuid` = {}",
            player->GetGUID().GetCounter(), targetGuid.GetCounter()))
        {
            SendSocnMessage(handler, SOCN_ENEMY_ALREADY, targetName);
            return false;
        }

        if (QueryResult count = CharacterDatabase.Query(
            "SELECT COUNT(*) FROM `socn_enemies` WHERE `ownerGuid` = {}", player->GetGUID().GetCounter()))
        {
            if (count->Fetch()[0].Get<uint32>() >= g_maxEnemies)
            {
                SendSocnMessage(handler, SOCN_ENEMY_LIMIT, g_maxEnemies);
                return false;
            }
        }

        CharacterDatabase.Execute(
            "INSERT INTO `socn_enemies` (`ownerGuid`, `enemyGuid`) VALUES ({}, {})",
            player->GetGUID().GetCounter(), targetGuid.GetCounter());

        SendSocnMessage(handler, SOCN_ENEMY_SET, targetName);
        return true;
    }

    inline bool RemoveEnemy(Player* player, ObjectGuid targetGuid, std::string const& targetName,
        ChatHandler* handler)
    {
        QueryResult existing = CharacterDatabase.Query(
            "SELECT 1 FROM `socn_enemies` WHERE `ownerGuid` = {} AND `enemyGuid` = {}",
            player->GetGUID().GetCounter(), targetGuid.GetCounter());
        if (!existing)
        {
            SendSocnMessage(handler, SOCN_ENEMY_NOT_YOURS, targetName);
            return false;
        }

        CharacterDatabase.Execute(
            "DELETE FROM `socn_enemies` WHERE `ownerGuid` = {} AND `enemyGuid` = {}",
            player->GetGUID().GetCounter(), targetGuid.GetCounter());

        SendSocnMessage(handler, SOCN_ENEMY_REMOVED, targetName);
        return true;
    }
}

#endif // MOD_SOCIAL_NETWORK_H
