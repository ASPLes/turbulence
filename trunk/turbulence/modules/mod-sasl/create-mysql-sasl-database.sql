-- create database
create user turbulence_test identified by '1234';
update mysql.user set host = 'localhost' where user = 'turbulence_test' and host = '%';
create database turbulence_test;
grant all on turbulence_test.* TO turbulence_test@localhost;

-- connect to the database
use turbulence_test

-- Create users database
create table users (id  INT AUTO_INCREMENT PRIMARY KEY)  engine = innodb, charset = utf8;
alter table users add column auth_id varchar (255);
alter table users add column password varchar (255);
alter table users add column is_active int default 1;
alter table users add column serverName varchar(255);

-- Insert some test users (user: aspl, password: test)
insert into users (auth_id, password) values ('aspl', '09:8F:6B:CD:46:21:D3:73:CA:DE:4E:83:26:27:B4:F6');




 