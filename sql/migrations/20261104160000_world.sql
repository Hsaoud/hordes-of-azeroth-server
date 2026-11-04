DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261104160000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261104160000');

UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'Enemies on your Consecration take double, and enemies that strike you take double.', 'Enemies that strike you or stand on your Consecration take double.') WHERE `id` IN (810, 833, 834);
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'Deals 50% more damage while you are Enraged.', 'Deals 50% more damage while Enraged.') WHERE `id` IN (304, 319, 320);
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'Each enemy within reach adds', 'Each enemy hit adds') WHERE `id` IN (309, 329, 330);
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'Each flay of an enemy with 10 or more stacks of Decay heals', 'Flaying an enemy with 10 or more stacks of Decay heals') WHERE `id` IN (509, 530, 531);
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'per stack of Bleed on each, up to 100% more', 'per stack of Bleed on the target, up to 100% more') WHERE `id` IN (1006, 1024, 1025);
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'whichever it has more of', 'whichever is higher') WHERE `id` IN (907, 925, 926);
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'per stack of Poison on the enemy.', 'per stack of Poison on the target.') WHERE `id` IN (202, 224, 225);

END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
