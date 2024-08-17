#!/bin/sh

# Create man page (kdocker.1) from pod template
# $1 = input pod file
# $2 = output file
# $3 = version

if [ -z "$1" ]; then
    echo "Missing parameter - Input pod file"
    exit 1
fi

if [ -z "$2" ]; then
    echo "Missing parameter - output file"
    exit 1
fi

if [ -z "$3" ]; then
    echo "Missing parameter - version"
    exit 1
fi

pod2man --center  "General Commands Manual" \
        --release "Version $3"              \
        --date    "$(date +'%e %B, %Y')"    \
        $1  $2
