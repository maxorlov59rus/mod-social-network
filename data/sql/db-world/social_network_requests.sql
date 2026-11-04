-- Social Network module: friend requests list + friend marks in top.
-- Strings 82-84 (apply to acore_world; idempotent).

SET @MODULE := 'mod-social-network';

DELETE FROM `module_string` WHERE `module` = @MODULE AND `id` >= 82;
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
(@MODULE, 82, '=== Incoming friend requests ({}) ==='),
(@MODULE, 83, 'No incoming friend requests.'),
(@MODULE, 84, '{}. [Friend] {} - {} likes');

DELETE FROM `module_string_locale` WHERE `module` = @MODULE AND `id` >= 82 AND `locale` = 'ruRU';
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
(@MODULE, 82, 'ruRU', '=== Входящие заявки в друзья ({}) ==='),
(@MODULE, 83, 'ruRU', 'Входящих заявок в друзья нет.'),
(@MODULE, 84, 'ruRU', '{}. [Друг] {} — {} лайков');
