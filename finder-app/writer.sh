#!/bin/bash

writefile=$1
writestr=$2

if [ -d "$writefile" ]; then
    echo "$writefile is not a file"
    exit 1
fi

if [ $# -ne 2 ]; then
    echo "Incorrect number of arguments. Correct usage is: writer.sh File String"
    exit 1
fi

mkdir -p "$(dirname "$writefile")"  #create the parent directory if it does not exist

if [ $(echo "$writestr" > $writefile) ]; then
    echo "Could not create or modify file $writefile".
    exit 1
fi

