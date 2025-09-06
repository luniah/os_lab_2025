#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Ошибка: Не указаны аргументы"
    exit 1
fi

count=$#
sum=0

for num in "$@"; do
    sum=$(echo "$sum + $num" | bc -l)
done

average=$(echo "$sum / $count" | bc -l)

echo "Количество: $count"
echo "Среднее: $average"
