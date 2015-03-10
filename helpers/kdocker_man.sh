#!/bin/sh

# Create man page (kdocker.1) from pod template

pod2man --center  'General Commands Manual' \
        --release 'Version 4.9'             \
        --date    "$(date +'%e %B, %Y')"    \
        kdocker.pod  kdocker.1
