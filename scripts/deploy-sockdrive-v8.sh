#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: $0 <jsdos-file> <bundle-prefix>"
    exit 1
fi

rm -rf sockdrive
bundle="./sockdrive/$2-$(basename "$1")"
sockdrive sockify $1 $2- ./sockdrive https://v8.js-dos.com/sockdrive $bundle

aws s3 sync --endpoint-url=https://storage.yandexcloud.net --acl public-read sockdrive s3://jsdos/sockdrive
