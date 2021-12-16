#!/bin/sh
# wait-for-postgres.sh

set -e
  
host="$1"
shift

export PGUSER=$(grep 'storage.type.postgresql.username' $OWLS_CONFIG/owls.properties | awk -F '= ' '{print $2}')
export PGPASSWORD=$(grep 'storage.type.postgresql.password' $OWLS_CONFIG/owls.properties | awk -F '= ' '{print $2}')
  
until psql -h "$host" -c '\q'; do
  >&2 echo "Postgres is unavailable - sleeping"
  sleep 1
done
  
>&2 echo "Postgres is up - executing command"

if [ "$1" = '/openwifi/owls' -a "$(id -u)" = '0' ]; then
    if [ "$RUN_CHOWN" = 'true' ]; then
      chown -R "$OWLS_USER": "$OWLS_ROOT" "$OWLS_CONFIG"
    fi
    exec su-exec "$OWLS_USER" "$@"
fi

exec "$@"
