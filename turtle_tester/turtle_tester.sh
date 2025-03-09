#! /bin/bash

SLEEP_TIME=1

cd "$(dirname "$0")"
TURTLE_DIR=$(pwd)

cd ..
WEBSERV_DIR=$(pwd)
cd $TURTLE_DIR

rm -rf */logs */response
echo

for DIR in $(ls -d */); do

	DIR=${DIR%/}
	echo -n "$DIR:  "

	mkdir -p $DIR/logs $DIR/response

	if [[ ! -f "$DIR/conf.conf" || ! -f "$DIR/request.txt" ]]; then
		echo "❓  missing conf.conf or request.txt"
		continue
	fi



	# NGINX
	nginx -c "$PWD/$DIR/conf.conf" -p $WEBSERV_DIR -g "daemon off;" 2> "$DIR/logs/nginx.txt" & NGINX_PID=$!
	sleep $SLEEP_TIME;
	if ! ps -p $NGINX_PID > /dev/null 2>&1; then
		echo "❓  $(cat "$DIR/logs/nginx.txt")"
		continue
	fi
	{ tail -n +2 $DIR/request.txt; echo; echo; } | nc $(head -n 1 $DIR/request.txt) > $DIR/response/nginx.txt & NC_PID=$!
	sleep 0.1
	kill $NC_PID
	kill -SIGTERM $NGINX_PID
	wait $NGINX_PID


	# WEBSERV
	cd $WEBSERV_DIR
	./webserv "$TURTLE_DIR/$DIR/conf.conf" > /dev/null 2> "$TURTLE_DIR/$DIR/logs/webserv.txt" & WEBSERV_PID=$!
	sleep $SLEEP_TIME;
	if ! ps -p $WEBSERV_PID > /dev/null 2>&1; then
		echo "❓ $(cat "$TURTLE_DIR/$DIR/logs/webserv.txt")"
		cd $TURTLE_DIR
		continue
	fi
	{ tail -n +2 $TURTLE_DIR/$DIR/request.txt; echo; echo; } | nc $(head -n 1 $TURTLE_DIR/$DIR/request.txt) > $TURTLE_DIR/$DIR/response/webserv.txt & NC_PID=$!
	sleep 0.1
	kill $NC_PID
	kill -SIGINT $WEBSERV_PID
	wait $WEBSERV_PID
	cd $TURTLE_DIR




	# compare
	grep -vE "^(Server: |Date: |Last-Modified: |ETag: |Accept-Ranges: )" "$DIR/response/nginx.txt" > "$DIR/response/nginx.txt.tmp" && mv "$DIR/response/nginx.txt.tmp" "$DIR/response/nginx.txt"
	grep "" "$DIR/response/webserv.txt" > "$DIR/response/webserv.txt.tmp" && mv "$DIR/response/webserv.txt.tmp" "$DIR/response/webserv.txt"

	grep -vE "^(Content-Length: |<head><title>.*?</title></head>|<hr><center>nginx.*?</center>)" "$DIR/response/nginx.txt" > "$DIR/response/nginx.txt.tmp"
	grep -vE "^(Content-Length: |<head><meta charset=\"UTF-8\"><title>.*?</title></head>|<hr><center>🐢webserv🐢</center>)" "$DIR/response/webserv.txt" > "$DIR/response/webserv.txt.tmp"


	if cmp -s "$DIR/response/nginx.txt.tmp" "$DIR/response/webserv.txt.tmp"; then
		echo "✅  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt"
	else
		echo "❌  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt"
	fi

	rm -f "$DIR/response/nginx.txt.tmp" "$DIR/response/webserv.txt.tmp"

	SLEEP_TIME=0.2

done

echo
