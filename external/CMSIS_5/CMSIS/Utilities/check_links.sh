#!/bin/bash

linkchecker -F csv --check-extern $1

OFS=$IFS
IFS=$'\n'

for line in $(grep -E '^[^#]' linkchecker-out.csv | tail -n +2); do 
  link=$(echo $line | cut -d';' -f 1)
  file=$(echo $line | cut -d';' -f 2)
  msg=$(echo $line | cut -d';' -f 4)
  src=$(echo $file | sed -E 's/file:\/\/(.*)\/Documentation\/(\w+)\/.*/\1\/DoxyGen\/\2/')
  if [ -d $src ]; then
    origin=$(grep -Ern "href=['\"]${link}['\"]" $src/src/)
    for o in $origin; do
      ofile=$(echo $o | cut -d':' -f 1)
      oline=$(echo $o | cut -d':' -f 2)
      echo "${ofile}:${oline};${link};${msg}" >&2
    done
  fi
done

IFS=$OFS

exit 0
