BEGIN TRANSACTION;
DROP TABLE IF EXISTS `waypoint`;
CREATE TABLE IF NOT EXISTS `waypoint` (
	`pk1`	INTEGER NOT NULL,
	`pk2`	INTEGER NOT NULL,
	`v`	INTEGER,
	PRIMARY KEY(`pk1`,`pk2`)
);
DROP TABLE IF EXISTS `user`;
CREATE TABLE IF NOT EXISTS `user` (
	`user_id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`guid`	VARCHAR ( 36 ) NOT NULL,
	`name`	VARCHAR ( 80 ) NOT NULL DEFAULT 'Lee',
	`gold`	INTEGER NOT NULL DEFAULT '1000000',
	`updated`	DATETIME DEFAULT CURRENT_TIMESTAMP
);
DROP TABLE IF EXISTS `shipyard`;
CREATE TABLE IF NOT EXISTS `shipyard` (
	`shipyard_id`	INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
	`user_id`	INTEGER,
	`name`	TEXT,
	`x`	REAL,
	`y`	REAL
);
DROP TABLE IF EXISTS `shiproute`;
CREATE TABLE IF NOT EXISTS `shiproute` (
	`shiproute_id`	INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
	`port1_id`	INTEGER,
	`port2_id`	INTEGER
);
DROP TABLE IF EXISTS `ship`;
CREATE TABLE IF NOT EXISTS `ship` (
	`ship_id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`user_id`	INTEGER NOT NULL,
	`name`	VARCHAR ( 80 ) NOT NULL DEFAULT 'Bad Gateway',
	`x`	REAL NOT NULL DEFAULT '30',
	`y`	REAL NOT NULL DEFAULT '40',
	`angle`	REAL NOT NULL DEFAULT '1',
	`oil`	REAL NOT NULL DEFAULT '200000',
	`updated`	DATETIME DEFAULT CURRENT_TIMESTAMP,
	`shiproute_id`	INTEGER,
	`ship_type`	INTEGER,
	`docked_shipyard_id`	INTEGER,
	`captain_id`	INTEGER,
	`template_id`	INTEGER
);
DROP TABLE IF EXISTS `seaport`;
CREATE TABLE IF NOT EXISTS `seaport` (
	`seaport_id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`name`	VARCHAR ( 80 ) NOT NULL,
	`x`	REAL NOT NULL,
	`y`	REAL NOT NULL,
	`user_id`	INTEGER,
	`seaport_type`	INTEGER
);
DROP TABLE IF EXISTS `captain`;
CREATE TABLE IF NOT EXISTS `captain` (
	`captain_id`	INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
	`name`	TEXT,
	`template_id`	INTEGER,
	`user_id`	INTEGER
);
DROP TABLE IF EXISTS `account`;
CREATE TABLE IF NOT EXISTS `account` (
	`account_id`	TEXT NOT NULL UNIQUE,
	`s`	TEXT,
	`v`	TEXT,
	`created`	DATETIME DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(`account_id`)
);
DROP INDEX IF EXISTS `ux_guid`;
CREATE UNIQUE INDEX IF NOT EXISTS `ux_guid` ON `user` (
	`guid`
);
COMMIT;
