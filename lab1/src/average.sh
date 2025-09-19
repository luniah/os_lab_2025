#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Ошибка: Не указаны аргументы"
    exit 1
fi

sum=0
for num in "$@"; do
    sum=$((sum + num))
done

average=$((sum / $#))

echo "Количество: $#"
echo "Среднее: $average"
