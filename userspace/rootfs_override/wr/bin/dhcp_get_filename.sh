#!/bin/sh

# Adam Wujek @ CERN
#
# This script is to be run by udhcpc
#
# udhcpc should save URL to the dot-config in boot_file.
# URL shall be stored in "filename" configuration field of DHCP server.
#
# We create a temporary file in /tmp, to avoid wearing flash if not
# needed. Then we replace the real file if different.
tmpdir="/tmp"

if [ -n "$boot_file" ]; then
    echo $boot_file > "$tmpdir"/dot-config_source_url
fi
