#! /bin/bash
set +m

SLEEP_TIME=1

# Arguments
SKIP_COUNT=${1:-0}       # Number of tests to skip
TEST_LIMIT=${2:-0}       # Max number of tests to run (0 = unlimited)

cd "$(dirname "$0")"
TURTLE_DIR=$(pwd)

cd ..
WEBSERV_DIR=$(pwd)
cd $TURTLE_DIR

rm -rf */logs */response
echo

counter=0  # Directory counter
ran=0      # Test counter

for DIR in $(ls -d */); do

    # Skip the first N directories based on SKIP_COUNT
    if (( counter < SKIP_COUNT )); then
        ((counter++))
        continue
    fi

    # Stop after running TEST_LIMIT tests (if specified)
    if (( TEST_LIMIT > 0 && ran >= TEST_LIMIT )); then
        break
    fi

    ((counter++))
    ((ran++))

	DIR=${DIR%/}
	echo -n "$DIR:  "

	mkdir -p $DIR/logs $DIR/response

	if [[ ! -f "$DIR/conf.conf" || ! -f "$DIR/request.txt" ]]; then
		echo "❓  missing conf.conf or request.txt"
		continue
	fi

	chmod +x $PWD/$DIR/prepare.sh > /dev/null 2>&1
	chmod +x $PWD/$DIR/cleanup.sh > /dev/null 2>&1

	# NGINX
	(cd $PWD/$DIR && $PWD/prepare.sh > /dev/null 2>&1)
	nginx -c "$PWD/$DIR/conf.conf" -p $WEBSERV_DIR -g "daemon off;" 2> "$DIR/logs/nginx.txt" & NGINX_PID=$!
	disown $NGINX_PID
	sleep $SLEEP_TIME
	{ tail -n +2 $DIR/request.txt; echo; echo; } | nc $(head -n 1 $DIR/request.txt) 2> /dev/null > $DIR/response/nginx.txt & NC_PID=$!
	disown $NC_PID
	sleep 0.1
	kill $NC_PID 2> /dev/null
	if ! ps -p $NGINX_PID > /dev/null 2>&1; then
		echo "💥  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt    $DIR/logs/nginx.txt 🔍 $DIR/logs/webserv.txt"
		continue
	fi
	kill -SIGTERM $NGINX_PID 2> /dev/null
	wait $NGINX_PID 2> /dev/null
	(cd $PWD/$DIR && $PWD/cleanup.sh > /dev/null 2>&1)

	sleep $SLEEP_TIME

	# WEBSERV
	(cd $PWD/$DIR && $PWD/prepare.sh > /dev/null 2>&1)
	cd $WEBSERV_DIR
	./webserv "$TURTLE_DIR/$DIR/conf.conf" > /dev/null 2> "$TURTLE_DIR/$DIR/logs/webserv.txt" & WEBSERV_PID=$!
	disown $WEBSERV_PID
	sleep $SLEEP_TIME
	{ tail -n +2 $TURTLE_DIR/$DIR/request.txt; echo; echo; } | nc $(head -n 1 $TURTLE_DIR/$DIR/request.txt) 2> /dev/null > $TURTLE_DIR/$DIR/response/webserv.txt & NC_PID=$!
	disown $NC_PID
	sleep 0.1
	kill $NC_PID 2> /dev/null
	if ! ps -p $WEBSERV_PID > /dev/null 2>&1; then
		echo "💥  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt    $DIR/logs/nginx.txt 🔍 $DIR/logs/webserv.txt"
		cd $TURTLE_DIR
		continue
	fi
	kill -SIGINT $WEBSERV_PID 2> /dev/null
	wait $WEBSERV_PID 2> /dev/null
	cd $TURTLE_DIR
	(cd $PWD/$DIR && $PWD/cleanup.sh > /dev/null 2>&1)

	# compare
	grep -vE "^(Last-Modified: |ETag: |Accept-Ranges: )" "$DIR/response/nginx.txt" > "$DIR/response/nginx.txt.tmp" && mv "$DIR/response/nginx.txt.tmp" "$DIR/response/nginx.txt"
	grep "" "$DIR/response/webserv.txt" > "$DIR/response/webserv.txt.tmp" && mv "$DIR/response/webserv.txt.tmp" "$DIR/response/webserv.txt"

	grep -vE "^(Server: |Date: |Content-Length: |<hr><center>nginx/[0-9].*?</center>)" "$DIR/response/nginx.txt" > "$DIR/response/nginx.txt.tmp"
	grep -vE "^(Server: |Date: |Content-Length: |<meta charset=\"UTF-8\"><!--🐢-->|<hr><center>🐢webserv🐢</center>)" "$DIR/response/webserv.txt" > "$DIR/response/webserv.txt.tmp"

	if cmp -s "$DIR/response/nginx.txt.tmp" "$DIR/response/webserv.txt.tmp"; then
		echo "✅  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt    $DIR/logs/nginx.txt 🔍 $DIR/logs/webserv.txt"
	else
		echo "❌  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt    $DIR/logs/nginx.txt 🔍 $DIR/logs/webserv.txt"
	fi

	SLEEP_TIME=0.2

done

echo
