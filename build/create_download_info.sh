#!/bin/bash

# Author Adam Wujek
# Released according to the GNU GPL, version 2
#
# script to generate entries in file download-info
#
# Use: go to build directory then invoke script from there with build log
# as parameter
#
# NOTE: 

if [ "$#" -ne 1 ]; then
    echo "Please provide build log as parameter"
    exit 1
fi

if ! [ -d downloads ]; then
    echo "downloads directory does not exist in current directory"
    exit 1
fi

cat $1 | grep -- "^--"  | grep "://" | while read line;
do
    file_name=`echo $line | awk -F/ '{print $NF}'`
    link=`echo $line | awk '{print $NF}'`
    # print file name
    echo -n -e "$file_name \t"
    # print checksum
    echo -n `md5sum downloads/$file_name | awk '{print $1}'`
    echo " \\"
    # print link
    echo -e "   $link\n"
done
