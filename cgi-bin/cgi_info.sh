#! /bin/bash

asdfasdfsf
echo "Content-Type: text/plain"
echo "Connection: close"
echo "Content-Length: 99999999999999999999"
echo "Transfer-Encoding: chunkedxxx"
echo "asdf: asdf"
echo ""
echo "Status: 123 BANANA"
echo ""
echo "Hello world!"
echo ""
echo "All arguments: $@"
echo ""
echo "env:"
echo "$(env)"
echo ""
exit 1
