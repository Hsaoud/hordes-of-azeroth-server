DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261104140000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261104140000');

UPDATE `bonk_weapon` SET `damage_fraction` = 0.9 WHERE `id` IN (1003, 1018, 1019);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.3, `description` = REPLACE(`description`, 'Swipes all nearby enemies for damage.', 'Swipes all nearby enemies for damage, 10% more per stack of Bleed on each, up to 100% more.') WHERE `id` IN (1006, 1024, 1025);
UPDATE `bonk_weapon` SET `damage_fraction` = 0.7 WHERE `id` = 1020;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.2 WHERE `id` IN (1008, 1028, 1029);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.5 WHERE `id` IN (1011, 1034, 1035);

UPDATE `bonk_weapon` SET `damage_fraction` = 1.8 WHERE `id` = 703;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.2 WHERE `id` IN (718, 719);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.6 WHERE `id` = 702;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.1 WHERE `id` IN (716, 717);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.2 WHERE `id` = 704;
UPDATE `bonk_weapon` SET `damage_fraction` = 3.25 WHERE `id` = 705;
UPDATE `bonk_weapon` SET `damage_fraction` = 2.875 WHERE `id` IN (722, 723);

UPDATE `bonk_weapon` SET `damage_fraction` = 1.0, `apply_stacks` = 2, `description` = REPLACE(`description`, 'dealing damage and 1 stack of Burning', 'dealing damage and 2 stacks of Burning') WHERE `id` IN (909, 929, 930);
UPDATE `bonk_weapon` SET `damage_fraction` = 2.4, `description` = REPLACE(`description`, '4% more per stack of Decay on the target, up to 30 stacks', '4% more per stack of Decay or Burning on the target, whichever it has more of, up to 30 stacks') WHERE `id` IN (907, 925, 926);
UPDATE `bonk_weapon` SET `damage_fraction` = 0.45 WHERE `id` = 910;
UPDATE `bonk_weapon` SET `damage_fraction` = 0.3 WHERE `id` = 932;
UPDATE `bonk_weapon` SET `apply_stacks` = 2 WHERE `id` IN (905, 921, 922);
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'with 1 stack of Agony, or 2 on an uncursed enemy', 'with 2 stacks of Agony, or 3 on an uncursed enemy') WHERE `id` = 905;
UPDATE `bonk_weapon` SET `description` = REPLACE(`description`, 'the most health with 1 stack of Agony', 'the most health with 2 stacks of Agony') WHERE `id` IN (921, 922);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.1 WHERE `id` IN (906, 923, 924);

END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
