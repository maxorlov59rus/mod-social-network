-- Social Network module: repost strings 85-89 (apply to acore_world; idempotent).

SET @MODULE := 'mod-social-network';

DELETE FROM `module_string` WHERE `module` = @MODULE AND `id` >= 85;
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
(@MODULE, 85, 'Post {} reposted to your wall.'),
(@MODULE, 86, 'You cannot repost your own post.'),
(@MODULE, 87, 'You already reposted this post.'),
(@MODULE, 88, '[{}] {} (repost of {}''s post)'),
(@MODULE, 89, '{} reposted your post: "{}"');

DELETE FROM `module_string_locale` WHERE `module` = @MODULE AND `id` >= 85 AND `locale` = 'ruRU';
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
(@MODULE, 85, 'ruRU', 'Запись {} добавлена на вашу стену (репост).'),
(@MODULE, 86, 'ruRU', 'Нельзя репостить свою запись.'),
(@MODULE, 87, 'ruRU', 'Вы уже репостили эту запись.'),
(@MODULE, 88, 'ruRU', '[{}] {} (репост от {})'),
(@MODULE, 89, 'ruRU', '{} репостнул вашу запись: "{}"');
