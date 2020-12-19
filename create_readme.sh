#!/bin/bash

echo "# leetcode 题解"
echo "-----"
echo ""
echo "### Algorithm"
echo "- keep thinking and keep coding..."
echo "- C++ tianxiadiyi!"

echo "| # | Title | Solution | Difficulty |"
echo "|---| ----- | -------- | ---------- |"

cd cpp

for i in `ls`;
    do echo `../scripts/readme.sh $i` >> tmp;
done

for i in `awk '{split($1,a,"[\|\|]");print NR,a[2]}' tmp | sort -n -k2 | awk '{print $1}'`; 
do awk 'NR=='$i'{print}' tmp; 
done

rm -f tmp
cd ..
