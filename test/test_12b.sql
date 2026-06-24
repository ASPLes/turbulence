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
insert into users (auth_id, password, serverName) values ('aspl', '09:8F:6B:CD:46:21:D3:73:CA:DE:4E:83:26:27:B4:F6', 'test-12.server');
insert into users (auth_id, password, serverName) values ('aspl2', '09:8F:6B:CD:46:21:D3:73:CA:DE:4E:83:26:27:B4:F6', 'test-12.another-server');

-- user with a complex password containing characters that the old input
-- blacklist used to reject (';', single quote and '--'). Used by test_12c
-- to verify mod-sasl-mysql Option B (SQL escaping + no input blacklist).
-- plain text password is: s3cret;p'ass--w0rd  (md5 colon-hex below)
insert into users (auth_id, password, serverName) values ('special', '82:69:BD:33:6D:96:2C:B0:E5:62:C2:C3:86:1D:0E:18', 'test-12.server');

