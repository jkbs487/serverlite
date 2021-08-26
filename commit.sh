#!/bin/sh

if [ $# -lt 1 ]; then
    echo "usage: $0 message"
    exit 1
fi

MESSAGE=$@

git add .
git commit -m "${MESSAGE}"
git push github

if [ $? -eq 0 ]; then
    echo "push success!"
else
    echo "push fail!"
fi
