-- ============================================================
-- 1. Friends / enemies tables (apply to acore_characters)
-- ============================================================

CREATE TABLE IF NOT EXISTS `socn_friends` (
  `ownerGuid` int unsigned NOT NULL,
  `friendGuid` int unsigned NOT NULL,
  PRIMARY KEY (`ownerGuid`, `friendGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `socn_friend_requests` (
  `fromGuid` int unsigned NOT NULL,
  `toGuid` int unsigned NOT NULL,
  `createdAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`fromGuid`, `toGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `socn_enemies` (
  `ownerGuid` int unsigned NOT NULL,
  `enemyGuid` int unsigned NOT NULL,
  PRIMARY KEY (`ownerGuid`, `enemyGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- 2. Localized strings: friends / enemies / guild, ids 52-81
-- (apply to acore_world; idempotent)
-- ============================================================

SET @MODULE := 'mod-social-network';

DELETE FROM `module_string` WHERE `module` = @MODULE AND `id` >= 52;
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
(@MODULE, 52, 'Friend request sent to {}.'),
(@MODULE, 53, '{} sent you a friend request. Use .social friendaccept {}'),
(@MODULE, 54, '{} is now your friend.'),
(@MODULE, 55, '{} is already your friend.'),
(@MODULE, 56, 'A friend request to {} is already pending.'),
(@MODULE, 57, 'No pending friend request from {}.'),
(@MODULE, 58, '{} removed from friends.'),
(@MODULE, 59, '{} is not your friend.'),
(@MODULE, 60, 'You cannot add yourself.'),
(@MODULE, 61, '=== Friends ({}) ==='),
(@MODULE, 62, 'No friends yet. Send requests with .social friend <player>.'),
(@MODULE, 63, '{} added to enemies.'),
(@MODULE, 64, '{} removed from enemies.'),
(@MODULE, 65, 'You cannot add yourself.'),
(@MODULE, 66, '{} is not in your enemy list.'),
(@MODULE, 67, '=== Enemies ({}) ==='),
(@MODULE, 68, 'Enemy list is empty.'),
(@MODULE, 69, '{} is already in your enemy list.'),
(@MODULE, 70, 'Friends: {} | Enemies: {}'),
(@MODULE, 71, 'Guild online: {}'),
(@MODULE, 72, '=== Guild {} ==='),
(@MODULE, 73, 'Members: {}'),
(@MODULE, 74, 'Guild master: {}'),
(@MODULE, 75, '[{}] {} (online)'),
(@MODULE, 76, '[{}] {}'),
(@MODULE, 77, 'You are not in a guild.'),
(@MODULE, 78, 'Friend list is full ({}).'),
(@MODULE, 79, 'Enemy list is full ({}).'),
(@MODULE, 80, 'Friend request declined.'),
(@MODULE, 81, '{} is your friend - remove the friendship first.');

DELETE FROM `module_string_locale` WHERE `module` = @MODULE AND `id` >= 52 AND `locale` = 'ruRU';
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
(@MODULE, 52, 'ruRU', 'Заявка в друзья отправлена игроку {}.'),
(@MODULE, 53, 'ruRU', '{} отправил вам заявку в друзья. Принять: .social friendaccept {}'),
(@MODULE, 54, 'ruRU', '{} теперь ваш друг.'),
(@MODULE, 55, 'ruRU', '{} уже ваш друг.'),
(@MODULE, 56, 'ruRU', 'Заявка в друзья для {} уже отправлена.'),
(@MODULE, 57, 'ruRU', 'Заявки в друзья от {} нет.'),
(@MODULE, 58, 'ruRU', '{} удалён из друзей.'),
(@MODULE, 59, 'ruRU', '{} не в списке ваших друзей.'),
(@MODULE, 60, 'ruRU', 'Нельзя добавить себя.'),
(@MODULE, 61, 'ruRU', '=== Друзья ({}) ==='),
(@MODULE, 62, 'ruRU', 'Друзей пока нет. Отправьте заявку: .social friend <игрок>.'),
(@MODULE, 63, 'ruRU', '{} добавлен во враги.'),
(@MODULE, 64, 'ruRU', '{} удалён из врагов.'),
(@MODULE, 65, 'ruRU', 'Нельзя добавить себя.'),
(@MODULE, 66, 'ruRU', '{} нет в списке врагов.'),
(@MODULE, 67, 'ruRU', '=== Враги ({}) ==='),
(@MODULE, 68, 'ruRU', 'Список врагов пуст.'),
(@MODULE, 69, 'ruRU', '{} уже в списке врагов.'),
(@MODULE, 70, 'ruRU', 'Друзья: {} | Враги: {}'),
(@MODULE, 71, 'ruRU', 'Гильдия онлайн: {}'),
(@MODULE, 72, 'ruRU', '=== Гильдия {} ==='),
(@MODULE, 73, 'ruRU', 'Участники: {}'),
(@MODULE, 74, 'ruRU', 'Гилдмастер: {}'),
(@MODULE, 75, 'ruRU', '[{}] {} (в сети)'),
(@MODULE, 76, 'ruRU', '[{}] {}'),
(@MODULE, 77, 'ruRU', 'Вы не состоите в гильдии.'),
(@MODULE, 78, 'ruRU', 'Список друзей полон ({}).'),
(@MODULE, 79, 'ruRU', 'Список врагов полон ({}).'),
(@MODULE, 80, 'ruRU', 'Заявка в друзья отклонена.'),
(@MODULE, 81, 'ruRU', '{} ваш друг — сначала удалите дружбу.');
