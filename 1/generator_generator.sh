#!/bin/bash

n=$1

for i in $(seq 1 $n); do
   new_file="test${i}.txt"

   python3 generator.py -f $new_file -c 10000 -m 10000
done