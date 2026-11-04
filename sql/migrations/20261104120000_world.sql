DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261104120000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261104120000');

UPDATE `bonk_weapon` SET `damage_fraction` = 1.8 WHERE `id` IN (901, 913, 914, 902, 915, 916);
UPDATE `bonk_weapon` SET `damage_fraction` = 2.0 WHERE `id` IN (903, 917, 918);
UPDATE `bonk_weapon` SET `damage_fraction` = 0.45 WHERE `id` = 931;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.7 WHERE `id` IN (911, 912);

UPDATE `bonk_weapon` SET `damage_fraction` = 0.75 WHERE `id` IN (804, 823, 824);
UPDATE `bonk_weapon` SET `description` = 'Consecrates the ground beneath you for 8 sec, dealing Holy damage every sec to enemies on it. While standing on it you deal 15% more damage and heal for 0.25% of your maximum health every sec.' WHERE `id` = 804;
UPDATE `bonk_weapon` SET `description` = 'Consecrates the ground beneath you for 12 sec, dealing Holy damage every sec to enemies on it, up to 3 grounds at once. While standing on it you deal 25% more damage and heal for 0.25% of your maximum health every sec.' WHERE `id` = 823;
UPDATE `bonk_weapon` SET `description` = 'Consecrates the ground beneath you for 8 sec, dealing Holy damage every sec to enemies on it. While standing on it you deal 15% more damage and heal for 0.75% of your maximum health every sec.' WHERE `id` = 824;
UPDATE `bonk_weapon` SET `description` = 'Burns every enemy within 10 yards of you with holy light, dealing Holy damage every sec. Enemies on your Consecration take double, and enemies that strike you take double.' WHERE `id` = 810;
UPDATE `bonk_weapon` SET `description` = 'Burns every enemy within 12 yards of you with holy light, dealing Holy damage every sec. Enemies on your Consecration take double, and enemies that strike you take double.' WHERE `id` = 833;
UPDATE `bonk_weapon` SET `description` = 'Burns every enemy within 10 yards of you with holy light, dealing Holy damage every sec. Enemies on your Consecration take double, and enemies that strike you take double. You take 10% less damage.' WHERE `id` = 834;

UPDATE `bonk_weapon` SET `damage_fraction` = 2.8 WHERE `id` IN (607, 626, 627);
UPDATE `bonk_weapon` SET `damage_fraction` = 2.4 WHERE `id` IN (606, 624, 625);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.5 WHERE `id` IN (603, 618, 619);
UPDATE `bonk_weapon` SET `description` = 'Stabs the nearest enemy with both daggers for damage, each stab applying 1 stack of Poison, plus 1 for every 5 stacks it already has. Deals 30% more to Poisoned enemies. Stabbing an enemy with 10 or more stacks of Poison reduces the cooldown of Envenom by 0.5 sec.' WHERE `id` = 604;
UPDATE `bonk_weapon` SET `description` = 'Stabs the nearest enemy with both daggers for damage, each stab applying 1 stack of Poison, plus 1 for every 5 stacks it already has, and 2 stacks of Nature Vulnerability. Deals 30% more to Poisoned enemies. Stabbing an enemy with 10 or more stacks of Poison reduces the cooldown of Envenom by 0.5 sec.' WHERE `id` = 620;
UPDATE `bonk_weapon` SET `description` = 'Stabs the nearest enemy with both daggers for damage, 3 times below 35% health, each stab applying 1 stack of Poison, plus 1 for every 5 stacks it already has. Deals 30% more to Poisoned enemies. Stabbing an enemy with 10 or more stacks of Poison reduces the cooldown of Envenom by 0.5 sec.' WHERE `id` = 621;

UPDATE `bonk_weapon` SET `damage_fraction` = 0.32, `apply_stacks` = 3 WHERE `id` = 215;
UPDATE `bonk_weapon` SET `description` = 'Fires 3 arrows in a cone in front of you, dealing damage and applying 3 stacks of Poison.' WHERE `id` = 215;
UPDATE `bonk_weapon` SET `duration_ms` = 6000 WHERE `id` IN (204, 226);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.5, `duration_ms` = 3000 WHERE `id` = 227;
UPDATE `bonk_weapon` SET `description` = 'Places a trap that releases 4 vipers when an enemy comes close. Each viper rushes one of the 4 nearest enemies and bursts into a poison cloud for 6 sec. Enemies inside gain 1 stack of Poison every 0.5 sec.' WHERE `id` = 204;
UPDATE `bonk_weapon` SET `description` = 'Places a trap that releases 4 vipers when an enemy comes close. Each viper rushes one of the 4 nearest enemies and bursts into a poison cloud for 6 sec. Enemies inside gain 1 stack of Poison and 1 stack of Nature Vulnerability every 0.5 sec.' WHERE `id` = 226;
UPDATE `bonk_weapon` SET `description` = 'Places a trap that releases 4 vipers when an enemy comes close. Each viper rushes one of the 4 nearest enemies and explodes, dealing Nature damage and 5 stacks of Poison to enemies within 5 yards, then leaves a poison cloud for 3 sec. Enemies inside gain 1 stack of Poison every 0.5 sec.' WHERE `id` = 227;
UPDATE `bonk_weapon` SET `description` = 'Places a fire trap that explodes when an enemy comes close, dealing damage and burning the ground for 4 sec, applying Burning. Its explosion deals 5% more damage per stack of Poison on the enemy.' WHERE `id` = 202;
UPDATE `bonk_weapon` SET `description` = 'Places a fire trap that explodes when an enemy comes close, dealing damage and burning the ground for 4 sec, applying Burning. It also scatters 5 bomblets that explode for 40% damage within 3 yards, applying Burning. Its explosions deal 5% more damage per stack of Poison on the enemy.' WHERE `id` = 224;
UPDATE `bonk_weapon` SET `description` = 'Places a fire trap that explodes when an enemy comes close, dealing damage and burning the ground for 4 sec, applying Burning. 2 sec later it explodes again for 60% damage. Its explosions deal 5% more damage per stack of Poison on the enemy.' WHERE `id` = 225;
UPDATE `bonk_weapon` SET `damage_fraction` = 2.1 WHERE `id` IN (203, 217);
UPDATE `bonk_weapon` SET `damage_fraction` = 2.38, `behavior_value` = 0.56 WHERE `id` = 216;
UPDATE `bonk_weapon` SET `description` = 'Fires a shot that pierces every enemy in a line, dealing damage. Aims at the Marked group, or else the largest group. Each enemy pierced then explodes in turn for 56% of the damage within 4 yards. With an Explosive Arrow loaded, the shot explodes on its first hit, dealing its damage to all enemies within 8 yards.' WHERE `id` = 216;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.04 WHERE `id` = 214;

END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
