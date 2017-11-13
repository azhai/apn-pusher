CREATE TABLE IF NOT EXISTS `t_tokens` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `userid` bigint(14) unsigned NOT NULL DEFAULT 0,
  `gender` enum('M','F','X') CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL DEFAULT 'X',
  `package` varchar(50) DEFAULT NULL COMMENT '包名',
  `token` char(64) NOT NULL DEFAULT '',
  `voip_token` char(64) DEFAULT NULL,
  `universal_token` char(128) DEFAULT NULL,
  `version` varchar(30) NOT NULL DEFAULT '',
  `joined_at` timestamp NULL DEFAULT NULL COMMENT '添加时间',
  `is_invalid` tinyint(1) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `userid` (`userid`,`package`) USING BTREE,
  KEY `token` (`token`(8)) USING BTREE,
  KEY `joined_at` (`joined_at`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;


CREATE TABLE `t_tokens_0` LIKE `t_tokens`;
INSERT INTO `t_tokens_0` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '0%';
ALTER TABLE `t_tokens_0` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_1` LIKE `t_tokens`;
INSERT INTO `t_tokens_1` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '1%';
ALTER TABLE `t_tokens_1` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_2` LIKE `t_tokens`;
INSERT INTO `t_tokens_2` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '2%';
ALTER TABLE `t_tokens_2` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_3` LIKE `t_tokens`;
INSERT INTO `t_tokens_3` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '3%';
ALTER TABLE `t_tokens_3` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_4` LIKE `t_tokens`;
INSERT INTO `t_tokens_4` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '4%';
ALTER TABLE `t_tokens_4` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_5` LIKE `t_tokens`;
INSERT INTO `t_tokens_5` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '5%';
ALTER TABLE `t_tokens_5` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_6` LIKE `t_tokens`;
INSERT INTO `t_tokens_6` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '6%';
ALTER TABLE `t_tokens_6` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_7` LIKE `t_tokens`;
INSERT INTO `t_tokens_7` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '7%';
ALTER TABLE `t_tokens_7` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_8` LIKE `t_tokens`;
INSERT INTO `t_tokens_8` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '8%';
ALTER TABLE `t_tokens_8` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_9` LIKE `t_tokens`;
INSERT INTO `t_tokens_9` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE '9%';
ALTER TABLE `t_tokens_9` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_a` LIKE `t_tokens`;
INSERT INTO `t_tokens_a` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE 'a%';
ALTER TABLE `t_tokens_a` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_b` LIKE `t_tokens`;
INSERT INTO `t_tokens_b` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE 'b%';
ALTER TABLE `t_tokens_b` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_c` LIKE `t_tokens`;
INSERT INTO `t_tokens_c` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE 'c%';
ALTER TABLE `t_tokens_c` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_d` LIKE `t_tokens`;
INSERT INTO `t_tokens_d` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE 'd%';
ALTER TABLE `t_tokens_d` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_e` LIKE `t_tokens`;
INSERT INTO `t_tokens_e` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE 'e%';
ALTER TABLE `t_tokens_e` AUTO_INCREMENT=1;
CREATE TABLE `t_tokens_f` LIKE `t_tokens`;
INSERT INTO `t_tokens_f` SELECT NULL,userid,gender,package,token,voip_token,universal_token,version,joined_at,is_invalid FROM `t_tokens` WHERE token LIKE 'f%';
ALTER TABLE `t_tokens_f` AUTO_INCREMENT=1;
