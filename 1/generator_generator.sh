#!/bin/bash

n=$1

for i in $(seq 1 $n); do
   new_file="test${i}.txt"

   python3 generator.py -f $new_file -c 40000 -m 100000
done