#! /bin/bash

cd "$(dirname "$0")"

rm -rf */logs */response
echo

for DIR in $(ls -d */); do

	DIR=${DIR%/}
	echo -n "$DIR:  "

	mkdir -p $DIR/logs $DIR/response


	# NGINX
	nginx -c "$PWD/$DIR/conf.conf" -g "daemon off;" 2> "$DIR/logs/nginx.txt" & NGINX_PID=$!
	sleep 1;
	if ! ps -p $NGINX_PID > /dev/null 2>&1; then
		echo "❓  $(cat "$DIR/logs/nginx.txt")"
		continue
	fi
	{ tail -n +2 $DIR/request.txt; echo; echo; } | nc $(head -n 1 $DIR/request.txt) > $DIR/response/nginx.txt
	kill -SIGINT $NGINX_PID
	wait $NGINX_PID


	# WEBSERV
	../webserv "$PWD/$DIR/conf.conf" > /dev/null 2> "$DIR/logs/webserv.txt" & WEBSERV_PID=$!
	sleep 1;
	if ! ps -p $WEBSERV_PID > /dev/null 2>&1; then
		echo "❓ $(cat "$DIR/logs/webserv.txt")"
		continue
	fi
	{ tail -n +2 $DIR/request.txt; echo; echo; } | nc $(head -n 1 $DIR/request.txt) > $DIR/response/webserv.txt
	kill -SIGINT $WEBSERV_PID
	wait $WEBSERV_PID


	# compare
	grep -vE "^(Server: |Date: |Last-Modified: |ETag: |Accept-Ranges: )" "$DIR/response/nginx.txt" > "$DIR/response/nginx.txt.tmp" && mv "$DIR/response/nginx.txt.tmp" "$DIR/response/nginx.txt"
	grep "" "$DIR/response/webserv.txt" > "$DIR/response/webserv.txt.tmp" && mv "$DIR/response/webserv.txt.tmp" "$DIR/response/webserv.txt"

	if cmp -s "$DIR/response/nginx.txt" "$DIR/response/webserv.txt"; then
		echo "✅  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt"
	else
		echo "❌  $DIR/response/nginx.txt 🔍 $DIR/response/webserv.txt    🔧 $DIR/conf.conf    📝 $DIR/request.txt"
	fi


done

echo
