#!/bin/bash

for file in test*; do
    if [ -f "$file" ]; then
        echo "Checking $file"
        python3 checker.py -f "$file"
    fi
done

echo "Checking sum.txt"
python3 checker.py -f sum.txt
