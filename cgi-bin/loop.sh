#!/bin/bash

echo "Content-Type: text/plain"
echo ""

while true; do
    read -r line
    [[ -n "$line" ]] && echo "$line"
	sleep 1
done
