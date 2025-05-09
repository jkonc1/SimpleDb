if [ "$#" -ne 1 ]; then
    echo "Usage: ./unix_connect.sh <socket path>"
    exit 0
fi

socat - "UNIX-CONNECT:$1"