-- CREATE USER IF NOT EXISTS 'tinywebserver'@'localhost' IDENTIFIED BY 'tinywebserver';
-- CREATE DATABASE IF NOT EXISTS tinywebserver OWNER 'tinywebserver';
USE mysql;
update IGNORE user set host = '%' where user = 'root';
update IGNORE user set host = '%' where user = 'tinywebserver';
flush privileges;


create database yourdb;
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    passwd char(50) NULL
)ENGINE=InnoDB;
GRANT ALL PRIVILEGES ON yourdb.* TO 'tinywebserver'@'%';

-- INSERT INTO user(username, passwd) VALUES('name', 'passwd');
