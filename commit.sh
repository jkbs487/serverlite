#!/bin/sh

if [ $# -lt 1 ]; then
    echo "usage: $0 message"
    exit 1
fi

MESSAGE=$@

git add .
git commit -m "${MESSAGE}"
git push

if [ $! -eq 0 ]; then
    echo "提交成功"
else
    echo "提交失败"
fi
