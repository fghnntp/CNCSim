#!/bin/bash
# This script probe the files lines and words for a given dir
#

# determine which dir to probe
if [ $# -gt 1 ]
then
  echo "wrong use please given a dir or without parameters"
  exit 1
fi

if [ $# -eq 0 ]
then
  probe_dir="."
else
  probe_dir=$1
fi

if [ -d $probe_dir ]
then
  echo -n  ""
else
  echo "not given a dir"
  exit 1
fi

echo "probe_dir is $probe_dir"

files_count=0
lines_count=0
words_count=0
read_dir() {
  # statistic the files lines and words
  for file in $(ls -a $1)
  do
    if [ -d $1"/"$file ]
    then
      if [[ $file != "." && $file != ".." ]]
      then
        echo $1"/"$file
        read_dir $1"/"$file
      fi
    else
      local lines=0
      local words=0
      files_count=$[ $files_count + 1 ]
      lines=$(wc -l $1"/"$file|awk '{print $1}')
      words=$(wc -w $1"/"$file|awk '{print $1}')
      lines_count=$[ $lines_count + $lines ]
      words_count=$[ $words_count + $words ]
    fi
  done
}

read_dir $probe_dir

echo "files = $files_count"
echo "lines = $lines_count"
echo "words = $words_count"



