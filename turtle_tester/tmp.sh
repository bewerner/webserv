#!/bin/bash

for old_num in {28..35}; do
  new_num=$((old_num + 1))
  cp -r "b0$old_num" "b0$new_num"
  sed -i '' "s/b0$old_num/b0$new_num/g" "b0$new_num/conf.conf"
done
