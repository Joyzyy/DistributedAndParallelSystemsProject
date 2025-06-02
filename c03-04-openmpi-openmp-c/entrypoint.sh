#!/bin/bash

service ssh start

if [ "$HOSTNAME" = "c04" ]; then
    while [ ! -f /shared_keys/id_rsa.pub ]; do
        sleep 1
    done
    cat /shared_keys/id_rsa.pub >> /root/.ssh/authorized_keys
    echo "c04: SSH keys configured"
fi

if [ $# -eq 0 ]; then
    echo "Container $HOSTNAME is ready. Keeping alive..."
    tail -f /dev/null
else
    exec "$@"
fi
