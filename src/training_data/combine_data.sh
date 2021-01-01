#!/bin/bash

rm all_data all_data.tar.gz

ls * | grep -v gz | grep -v all | grep -v '~' | grep -v combine | while read file; do
    lines=$( cat $file | wc -l)
    head -n $((lines-1)) $file
done > all_data 

tar -czf all_data.tar.gz all_data
