#! /bin/bash

# asdfasdfsf
echo "Content-Type: text/plain"
# echo "text/plain"
# echo "Connection: close"
echo "Content-Length: 99999999999999999999"
echo "Transfer-Encoding: chunkedxxx"
echo "asdf: asdf"
echo "Status: 99 BANANA"
echo ""
echo "Hello world!"
echo ""
echo "All arguments: $@"
echo ""
echo "env:"
echo "$(env)"
echo ""
