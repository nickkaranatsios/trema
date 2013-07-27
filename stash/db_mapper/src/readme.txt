create user 'test'@'localhost' identified by 'test'
grant all on '*.*' to 'test'@'localhost'
The above two statement need to be executed with root privileges. 
Once executed successfully would allow user test to be the same as root.
To verify the create database command
mysql> show create database `ncdb`
