# 1. Define variables
INTERCEPT_LIB="$(pwd)/malloc_interceptor.so"
INTERCEPT_LIB="$(pwd)/malloc_interceptor.so"
MYSQL_USER="ics"
MYSQL_PASSWORD="ics"
MYSQL_PORT=3306
TEST_DB="test_db"

# 3. Start MySQL service
echo "Starting MySQL service..."
sudo service mysql start

# 4. Configure MySQL initial user and database
echo "Configuring MySQL..."
mysql -u $MYSQL_USER -e "ALTER USER 'ics'@'localhost' IDENTIFIED WITH 'mysql_native_password' BY '$MYSQL_PASSWORD';"
mysql -u $MYSQL_USER -p$MYSQL_PASSWORD -e "CREATE DATABASE $TEST_DB;"

# 5. Set LD_PRELOAD to intercept malloc/free/realloc
export LD_PRELOAD="$INTERCEPT_LIB"
echo "LD_PRELOAD set to: $INTERCEPT_LIB"

# 6. Test MySQL performance using sysbench
echo "Running sysbench tests..."
sysbench /usr/share/sysbench/oltp_read_write.lua \
  --mysql-host=127.0.0.1 \
  --mysql-port=$MYSQL_PORT \
  --mysql-user=$MYSQL_USER \
  --mysql-password=$MYSQL_PASSWORD \
  --mysql-db=$TEST_DB \
  --tables=1 \
  --table-size=100 \
  --threads=4 \
  --time=30 \
  --report-interval=5 \
  prepare

sysbench /usr/share/sysbench/oltp_read_write.lua \
  --mysql-host=127.0.0.1 \
  --mysql-port=$MYSQL_PORT \
  --mysql-user=$MYSQL_USER \
  --mysql-password=$MYSQL_PASSWORD \
  --mysql-db=$TEST_DB \
  --tables=1 \
  --table-size=1 \
  --threads=4 \
  --time=30 \
  --report-interval=5 \
  run

sysbench /usr/share/sysbench/oltp_read_write.lua \
  --mysql-host=127.0.0.1 \
  --mysql-port=$MYSQL_PORT \
  --mysql-user=$MYSQL_USER \
  --mysql-password=$MYSQL_PASSWORD \
  --mysql-db=$TEST_DB \
  cleanup

# 7. Stop MySQL service
echo "Stopping MySQL service..."
service mysql stop

echo "Script completed!"