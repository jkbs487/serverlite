#!/bin/bash

echo "# leetcode 题解"
echo "-----"
echo ""
echo "### Algorithm"
echo "- keep thinking and keep coding..."
echo "- C++ tianxiadiyi!"
echo ""

echo "| # | Title | Solution | Difficulty |"
echo "|---| ----- | -------- | ---------- |"

for i in `ls cpp`;
    do echo `scripts/readme.sh cpp/$i` >> tmp;
done

for i in `ls python`;
    do echo `scripts/readme.sh python/$i` >> tmp;
done

echo `cat tmp`

for i in `awk '{split($1,a,"[\|\|]");print NR,a[2]}' tmp | sort -n -k2 | awk '{print $1}'`; 
do awk 'NR=='$i'{print}' tmp; 
done

rm -f tmp
cd ..
