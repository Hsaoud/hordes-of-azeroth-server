DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261104150000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261104150000');

UPDATE `bonk_weapon` SET `damage_fraction` = 1.8 WHERE `id` = 631;
UPDATE `bonk_weapon` SET `damage_fraction` = 3.15 WHERE `id` = 729;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.08 WHERE `id` = 228;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.14 WHERE `id` = 229;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.8 WHERE `id` = 318;
UPDATE `bonk_weapon` SET `damage_fraction` = 2.375 WHERE `id` = 920;
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'deals 60% extra as Holy damage', 'deals 54% extra as Holy damage') WHERE `id` = 821;
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'deals 30% extra as Holy damage, 20% more', 'deals 27% extra as Holy damage, 20% more') WHERE `id` = 822;

END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
