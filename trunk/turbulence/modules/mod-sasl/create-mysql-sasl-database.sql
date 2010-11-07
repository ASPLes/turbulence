-- create database
CREATE user turbulence_test IDENTIFIED BY '1234';
UPDATE mysql.user SET host = 'localhost' WHERE user = 'turbulence_test' AND host = '%';
CREATE database turbulence_test;
GRANT ALL ON turbulence_test.* TO turbulence_test@localhost;

-- connect to the database
USE turbulence_test

-- Create users database
CREATE TABLE users (id  INT AUTO_INCREMENT PRIMARY KEY)  engine = innodb, charset = utf8;
ALTER TABLE users ADD COLUMN auth_id varchar (255);
ALTER TABLE users ADD COLUMN password varchar (255);
ALTER TABLE users ADD COLUMN is_active int default 1;
ALTER TABLE users ADD COLUMN serverName varchar(255);

-- Insert some test users (user: aspl, password: test)
insert into users (auth_id, password) values ('aspl', '09:8F:6B:CD:46:21:D3:73:CA:DE:4E:83:26:27:B4:F6');

