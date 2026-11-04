-- Social Network module: repost feature migration.
-- Apply to acore_characters.
--
-- Adds the repostOf column to socn_posts: stores the id of the original
-- post (0 for regular posts). The repost stores a text snapshot, so the
-- wall survives deletion of the original post.

ALTER TABLE `socn_posts`
  ADD COLUMN `repostOf` int unsigned NOT NULL DEFAULT 0 AFTER `text`,
  ADD KEY `idx_socn_posts_repost` (`repostOf`);
