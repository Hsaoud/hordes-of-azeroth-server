DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261104170000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261104170000');

UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'Consecrates the ground beneath you for 8 sec', 'Consecrates the ground beneath you for 12 sec') WHERE `id` IN (804, 824);
UPDATE `bonk_weapon` SET `description` = REPLACE(REPLACE(`description`, 'Consecrates the ground beneath you for 12 sec', 'Consecrates the ground beneath you for 18 sec'), 'up to 3 grounds at once', 'up to 5 grounds at once') WHERE `id` = 823;

END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
