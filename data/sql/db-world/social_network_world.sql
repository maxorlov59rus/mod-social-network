-- Social Network module: world data (acore_world).
-- Social Network Terminal — an interactable NPC that opens the module menu
-- (gossip UI) in the client. Uses the same "invisible vendor"-style pattern
-- as mod-wars-of-roses but with gossip: display 15435 ("Invisible Man"),
-- unit_flags 0x2 (non-attackable), flags_extra 0x2000 (out of combat).
-- Idempotent: DELETE before every INSERT.

SET @TERMINAL := 911100;
SET @TERMINAL_GUID := 91110001;
SET @SRC_VENDOR := (SELECT `entry` FROM `creature_template` WHERE `npcflag` & 1 LIMIT 1);

-- ============================================================
-- Creature template (gossip giver), copied from a real NPC
-- ============================================================

CREATE TEMPORARY TABLE `socn_ct_tmp` AS SELECT * FROM `creature_template` WHERE `entry` = @SRC_VENDOR;

UPDATE `socn_ct_tmp` SET
    `entry` = @TERMINAL,
    `name` = 'Социальная Сеть',
    `subname` = 'Терминал общения',
    `faction` = 35,
    `npcflag` = 1,
    `unit_flags` = 2,
    `minlevel` = 80,
    `maxlevel` = 80,
    `rank` = 0,
    `lootid` = 0,
    `mingold` = 0,
    `maxgold` = 0,
    `spell1` = 0,
    `spell2` = 0,
    `spell3` = 0,
    `spell4` = 0,
    `AIName` = '',
    `MovementType` = 0,
    `ScriptName` = 'SocnTerminalCreature',
    `flags_extra` = (`flags_extra` & ~0x80) | 0x2000,
    `VerifiedBuild` = 0;

DELETE FROM `creature_template` WHERE `entry` = @TERMINAL;
INSERT INTO `creature_template` SELECT * FROM `socn_ct_tmp`;
DROP TEMPORARY TABLE `socn_ct_tmp`;

-- Invisible model 15435 (single row at Probability 1)
DELETE FROM `creature_template_model` WHERE `CreatureID` = @TERMINAL;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `Probability`, `VerifiedBuild`) VALUES
(@TERMINAL, 0, 15435, 1, 0);

-- ============================================================
-- Spawn: near the Stormwind Auction House (human hub, easy to find).
-- Adjust position_x/y/z to your server needs.
-- ============================================================

DELETE FROM `creature` WHERE `id` = @TERMINAL AND `guid` = @TERMINAL_GUID;
INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`) VALUES
(@TERMINAL_GUID, @TERMINAL, 0, 1, 1, -8833.0, 660.0, 95.4, 3.1, 300, 0, 0);
