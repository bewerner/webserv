#!/bin/bash

echo "Content-type: text/plain"
echo ""

UPLOAD_DIR="${DOCUMENT_ROOT}/uploads"
mkdir -p $UPLOAD_DIR
ls -1 "$UPLOAD_DIR"
