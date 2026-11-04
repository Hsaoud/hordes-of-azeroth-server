DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261104130000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261104130000');

UPDATE `bonk_weapon` SET `damage_fraction` = 1.1 WHERE `id` IN (304, 319, 320);
UPDATE `bonk_weapon` SET `description` = 'Strikes the 3 nearest enemies within 10 yards and heals you for 0.5% of your maximum health per enemy hit, up to 2.5%. Deals 50% more damage while you are Enraged. Its critical strikes Enrage you, granting 25% Haste for 4 sec.' WHERE `id` = 304;
UPDATE `bonk_weapon` SET `description` = 'Strikes the 3 nearest enemies within 8 yards for damage and heals you for 0.5% of your maximum health per enemy hit, up to 2.5%. Deals 50% more damage while you are Enraged. Healing past full health becomes a Blood Shield that explodes for 200% damage within 8 yards when it breaks.' WHERE `id` = 319;
UPDATE `bonk_weapon` SET `description` = 'Strikes the 3 nearest enemies within 8 yards for damage and heals you for 0.5% of your maximum health per enemy hit, up to 2.5%. Deals 50% more damage while you are Enraged. Below 50% health it strikes all enemies within 8 yards instead and heals up to 4%.' WHERE `id` = 320;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.0 WHERE `id` IN (305, 321, 322);
UPDATE `bonk_weapon` SET `description` = 'When you are struck, strikes all enemies within 10 yards of you, at most once every 2 sec. Each enemy within reach adds 2% of your maximum health to your Shield, up to 6%.' WHERE `id` = 309;
UPDATE `bonk_weapon` SET `description` = 'When you are struck, strikes all enemies within 6 yards of you for damage, at most once every 2 sec. Each enemy within reach adds 2% of your maximum health to your Shield, up to 6%. Health damage you take is stored, up to 50% of your maximum health, and adds up to 200% damage to the next strike.' WHERE `id` = 329;
UPDATE `bonk_weapon` SET `description` = 'When you are struck, strikes all enemies within 6 yards of you for damage and Enrages you (25% Haste for 4 sec), at most once every 2 sec. Each enemy within reach adds 2% of your maximum health to your Shield, up to 6%.' WHERE `id` = 330;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.0 WHERE `id` IN (308, 327, 328);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.9 WHERE `id` IN (310, 331, 332);
UPDATE `bonk_weapon` SET `apply_stacks` = 4 WHERE `id` IN (301, 314);
UPDATE `bonk_weapon` SET `description` = 'Wounds the 3 nearest enemies within 10 yards, applying 4 stacks of Bleed. Your Cleave, Whirlwind and Thunder Clap hits also apply 1 stack of Bleed.' WHERE `id` = 301;
UPDATE `bonk_weapon` SET `description` = 'Wounds the 3 nearest enemies within 10 yards with 4 stacks of Bleed. Bleeding enemies that die leave a pool of blood for 4 sec that adds 1 stack of Bleed every sec to enemies within 4 yards. Your Cleave, Whirlwind and Thunder Clap hits also apply 1 stack of Bleed.' WHERE `id` = 314;
UPDATE `bonk_weapon` SET `apply_stacks` = 3 WHERE `id` IN (302, 315, 316);
UPDATE `bonk_weapon` SET `description` = 'Your critical strikes wound the enemy with 3 stacks of Bleed, at most once every 0.5 sec per enemy. Your critical strike chance against a Bleeding enemy is increased by 1% per stack of Bleed, up to 20%.' WHERE `id` = 302;
UPDATE `bonk_weapon` SET `description` = 'Your critical strikes wound the enemy and every enemy within 4 yards of it with 3 stacks of Bleed, at most once every 0.5 sec per enemy. Your critical strike chance against a Bleeding enemy is increased by 1% per stack of Bleed, up to 20%.' WHERE `id` = 315;
UPDATE `bonk_weapon` SET `description` = 'Your critical strikes wound the enemy with 3 stacks of Bleed, at most once every 0.5 sec per enemy. Each time this Bleed deals damage, it deals 10% more, up to triple. Your critical strike chance against a Bleeding enemy is increased by 1% per stack of Bleed, up to 20%.' WHERE `id` = 316;

UPDATE `bonk_weapon` SET `damage_fraction` = 0.35 WHERE `id` IN (509, 530, 531);
UPDATE `bonk_weapon` SET `description` = 'Flays the nearest enemy within 20 yards, dealing Shadow damage, applying 1 stack of Decay and slowing it by 30%. Keeps flaying it until it dies or leaves range. Deals 5% more per stack of Decay on the target, up to 30 stacks. Each flay of an enemy with 10 or more stacks of Decay heals you for 0.2% of your maximum health.' WHERE `id` = 509;
UPDATE `bonk_weapon` SET `description` = 'Flays the nearest enemy, dealing Shadow damage to it and half to enemies within 6 yards, applying 1 stack of Decay and slowing it by 30%. Deals 5% more per stack of Decay on the target, up to 30 stacks. Each flay of an enemy with 10 or more stacks of Decay heals you for 0.2% of your maximum health.' WHERE `id` = 530;
UPDATE `bonk_weapon` SET `description` = 'Flays the nearest enemy, dealing Shadow damage, applying 1 stack of Decay and slowing it by 30%. When it dies, its Decay moves to your next target. Deals 5% more per stack of Decay on the target, up to 30 stacks. Each flay of an enemy with 10 or more stacks of Decay heals you for 0.2% of your maximum health.' WHERE `id` = 531;
UPDATE `bonk_weapon` SET `damage_fraction` = 2.4 WHERE `id` = 502;
UPDATE `bonk_weapon` SET `damage_fraction` = 0.9 WHERE `id` = 516;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.6 WHERE `id` = 517;
UPDATE `bonk_weapon` SET `damage_fraction` = 4.9 WHERE `id` = 507;
UPDATE `bonk_weapon` SET `damage_fraction` = 5.6 WHERE `id` IN (526, 527);

UPDATE `bonk_weapon` SET `description` = 'Launches 5 Arcane Missiles at the nearest enemy over 1 sec, each dealing Arcane damage, 50% more to elites, mini-bosses and bosses. After each Blink, fires all 5 at once at the nearest enemies.' WHERE `id` = 409;
UPDATE `bonk_weapon` SET `description` = 'Launches 5 Arcane Missiles at the nearest enemy over 1 sec, each dealing Arcane damage, 50% more to elites, mini-bosses and bosses. With Blink, missiles pile up while you move instead, up to 40, and your next Blink fires them all at the nearest enemies.' WHERE `id` = 429;
UPDATE `bonk_weapon` SET `description` = 'Launches 5 Arcane Missiles at the nearest enemy over 1 sec, each dealing Arcane damage, 50% more to elites, mini-bosses and bosses. After each Blink, it fires all 5 at once at the nearest enemies and your spells deal 30% more damage for 4 sec.' WHERE `id` = 430;
UPDATE `bonk_weapon` SET `damage_fraction` = 0.6 WHERE `id` IN (401, 413, 414);
UPDATE `bonk_weapon` SET `description` = 'Ice shards pelt the largest group of enemies for 4 sec, dealing Frost damage and applying 1 stack of Chill every sec. Normal enemies Frozen inside stay Frozen until it ends, and Frozen enemies inside take 30% more Frost damage.' WHERE `id` = 413;
UPDATE `bonk_weapon` SET `damage_fraction` = 1.2 WHERE `id` IN (403, 417, 418);
UPDATE `bonk_weapon` SET `damage_fraction` = 1.3 WHERE `id` IN (404, 419, 420);

END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
