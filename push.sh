#!/bin/sh

if [ $# -lt 1 ];then
    echo "usage: $0 message"
    exit 1
fi

MESSAGE=$@

git add .
git commit -m "${MESSAGE}"
git push
