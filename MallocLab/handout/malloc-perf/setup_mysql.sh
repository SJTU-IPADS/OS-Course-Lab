#!/bin/bash

set -e  # Exit on error

MYSQL_USER="ics"
MYSQL_PASSWORD="ics"
MYSQL_PORT=3306

# 1. Install MySQL and test tools
echo "Installing MySQL and sysbench..."
sudo apt-get update -y
sudo apt-get install -y mysql-server sysbench

# 2. Start MySQL service
echo "Starting MySQL service..."
sudo service mysql start

# 3. Create MySQL user
echo "Creating MySQL user..."
echo 'MySQL_USER: ' $MYSQL_USER
echo 'MYSQL_PASSWORD: ' $MYSQL_PASSWORD
sudo mysql -e "CREATE USER '${MYSQL_USER}'@'localhost' IDENTIFIED BY '${MYSQL_PASSWORD}';"
sudo mysql -e "GRANT ALL PRIVILEGES ON *.* TO '${MYSQL_USER}'@'localhost' WITH GRANT OPTION;"
sudo mysql -e "FLUSH PRIVILEGES;"

sudo chown mysql:mysql /var/run/mysqld
sudo chmod 755 /var/run/mysqld

sudo service mysql stop
echo "MySQL setup completed!"