#!/bin/sh

filesdir=$1
searchstr=$2

if [ -f "$filesdir" ]; then
    echo "$filesdir is not a directory"
    exit 1
fi

if [ $# -ne 2 ]; then
    echo "Incorrect number of arguments. Correct usage is: finder.sh Directory SearchString"
    exit 1
fi

filelist=$(find "$filesdir" ! -type d)
numfiles=$(echo "$filelist" | wc -l ) 
numlines=$(grep "$searchstr" $filelist | wc -l)


echo "The number of files are $numfiles and the number of matching lines are $numlines"
exit 0
