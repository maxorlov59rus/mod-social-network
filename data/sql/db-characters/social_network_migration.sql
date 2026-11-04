-- Social Network module: migration for existing installations.
-- Apply to acore_characters.
--
-- Widens the profile status column: since emoticons are expanded into
-- |T...|t texture codes server-side (up to ~40 bytes per icon), the old
-- varchar(64) was too small for a 60-character status containing icons.
-- Code enforces a 255-byte limit on all user texts (ValidateText).

ALTER TABLE `socn_profiles` MODIFY `status` varchar(255) NOT NULL DEFAULT '';
