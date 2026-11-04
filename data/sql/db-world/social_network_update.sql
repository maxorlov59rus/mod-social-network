-- ============================================================
-- 1. New tables (apply to acore_characters)
-- ============================================================

CREATE TABLE IF NOT EXISTS `socn_messages` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `senderGuid` int unsigned NOT NULL,
  `receiverGuid` int unsigned NOT NULL,
  `text` varchar(255) NOT NULL,
  `createdAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_socn_messages_receiver` (`receiverGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `socn_comments` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `postId` int unsigned NOT NULL,
  `authorGuid` int unsigned NOT NULL,
  `text` varchar(255) NOT NULL,
  `createdAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_socn_comments_post` (`postId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- 2. Localized strings: new gossip/message/comment ids 32-51
-- (apply to acore_world; idempotent)
-- ============================================================

SET @MODULE := 'mod-social-network';

DELETE FROM `module_string` WHERE `module` = @MODULE AND `id` >= 32;
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
(@MODULE, 32, 'Social Network'),
(@MODULE, 33, 'My profile'),
(@MODULE, 34, 'My wall'),
(@MODULE, 35, 'Write a post (use .social post <text>)'),
(@MODULE, 36, 'Inbox'),
(@MODULE, 37, 'Top by likes'),
(@MODULE, 38, 'Close'),
(@MODULE, 39, 'Message sent to {}.'),
(@MODULE, 40, 'You cannot message yourself.'),
(@MODULE, 41, 'Inbox is empty.'),
(@MODULE, 42, '[{}] From {}: {}'),
(@MODULE, 43, 'Message {} deleted.'),
(@MODULE, 44, 'Comment added.'),
(@MODULE, 45, '=== Comments for post {} ==='),
(@MODULE, 46, '{}: {}'),
(@MODULE, 47, 'No comments yet.'),
(@MODULE, 48, 'Level: {}'),
(@MODULE, 49, 'Play time: {}'),
(@MODULE, 50, 'Message {} not found.'),
(@MODULE, 51, 'Commands: .social status <text> - status, .social post <text> - wall post, .social msg <player> <text> - private message, .social comment <postId> <text> - comment.');

DELETE FROM `module_string_locale` WHERE `module` = @MODULE AND `id` >= 32 AND `locale` = 'ruRU';
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
(@MODULE, 32, 'ruRU', 'Социальная Сеть'),
(@MODULE, 33, 'ruRU', 'Мой профиль'),
(@MODULE, 34, 'ruRU', 'Моя стена'),
(@MODULE, 35, 'ruRU', 'Написать запись (используйте .social post <текст>)'),
(@MODULE, 36, 'ruRU', 'Входящие'),
(@MODULE, 37, 'ruRU', 'Топ по лайкам'),
(@MODULE, 38, 'ruRU', 'Закрыть'),
(@MODULE, 39, 'ruRU', 'Сообщение отправлено игроку {}.'),
(@MODULE, 40, 'ruRU', 'Нельзя отправить сообщение самому себе.'),
(@MODULE, 41, 'ruRU', 'Входящие пусты.'),
(@MODULE, 42, 'ruRU', '[{}] От {}: {}'),
(@MODULE, 43, 'ruRU', 'Сообщение {} удалено.'),
(@MODULE, 44, 'ruRU', 'Комментарий добавлен.'),
(@MODULE, 45, 'ruRU', '=== Комментарии к записи {} ==='),
(@MODULE, 46, 'ruRU', '{}: {}'),
(@MODULE, 47, 'ruRU', 'Комментариев пока нет.'),
(@MODULE, 48, 'ruRU', 'Уровень: {}'),
(@MODULE, 49, 'ruRU', 'Время в игре: {}'),
(@MODULE, 50, 'ruRU', 'Сообщение {} не найдено.'),
(@MODULE, 51, 'ruRU', 'Команды: .social status <текст> — статус, .social post <текст> — запись на стену, .social msg <игрок> <текст> — личное сообщение, .social comment <id> <текст> — комментарий.');
