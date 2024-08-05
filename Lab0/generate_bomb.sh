#!/bin/bash

if [ ! -d "./warehouse" ]; then
    echo "Please run under the root directory of this lab!"
    exit -1
fi

total_bombs=30
student=$(cat ./student-number.txt)

if [ -z $student ]; then
    echo "Please enter your student number in student-number.txt at first! Otherwise you would recieve 0 POINTS!"
    exit -1
fi

bomb_id=$(($student%$total_bombs+1))

cp ./warehouse/bomb-${bomb_id} ./bomb
