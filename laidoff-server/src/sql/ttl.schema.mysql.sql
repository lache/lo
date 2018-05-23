
-- DROP TABLE IF EXISTS `mission`;
CREATE TABLE IF NOT EXISTS `mission` (
  `mission_id` int(11) NOT NULL AUTO_INCREMENT,
  `departure_id` int(11) NOT NULL,
  `arrival_id` int(11) NOT NULL,
  `reward` int(11) NOT NULL,
  `expired` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`mission_id`)
) ENGINE=InnoDB CHARSET=utf8mb4;

-- DROP TABLE IF EXISTS `region`;
CREATE TABLE IF NOT EXISTS `region` (
  `region_id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(80) NOT NULL,
  `x` float NOT NULL,
  `y` float NOT NULL,
  PRIMARY KEY (`region_id`)
) ENGINE=InnoDB CHARSET=utf8mb4;

-- DROP TABLE IF EXISTS `ship`;
CREATE TABLE IF NOT EXISTS `ship` (
  `ship_id` int(11) NOT NULL AUTO_INCREMENT,
  `user_id` int(11) NOT NULL,
  `name` varchar(80) NOT NULL DEFAULT 'Bad Gateway',
  `x` float NOT NULL DEFAULT '30',
  `y` float NOT NULL DEFAULT '40',
  `angle` float NOT NULL DEFAULT '1',
  `oil` float NOT NULL DEFAULT '200000',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`ship_id`)
) ENGINE=InnoDB CHARSET=utf8mb4;

-- DROP TABLE IF EXISTS `user`;
CREATE TABLE IF NOT EXISTS `user` (
  `user_id` int(11) NOT NULL AUTO_INCREMENT,
  `guid` varchar(36) NOT NULL,
  `name` varchar(80) NOT NULL DEFAULT 'Lee',
  `gold` int(11) NOT NULL DEFAULT '1000000',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`user_id`),
  UNIQUE KEY `ux_guid` (`guid`)
) ENGINE=InnoDB CHARSET=utf8mb4;
