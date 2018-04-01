#!/bin/bash

### PRINT SCRIPT USAGE
usage()
{
    echo " Usage: $0 [-p] [-t]"
    echo ""
    echo " Mandatory arguments:"
    echo "   -p            path to file where take the directory"
    echo "   -t      >=0   time in minutes since the creation/modification"
}

### FILE PARSING
take_word()
{
    while read line; do
            if [[ $line != \#* ]]; then
                    if [[ ${line:0:7} == "DirName" ]]; then
                        dir=$( echo "$line" | cut -d = -f 2 | tr -d ' ' )
                        remove $dir $time
                    fi
                fi
    done < "$path"
}

### MANAGE FILES ACCORDING MODIFY TIME
remove()
{
    if [[ $time -eq 0 ]]; then
        exec ls -l $dir
    else
        for item in $dir/*; do
            if [[ $(( $(date +%M) - $(date +%M -r $item) )) -ge $time ]]; then
                rm -rf $item
            fi
        done
    fi
}


### ""MAIN""
if [[ $# -ne 4 ]]; then
    usage
else

    while getopts ":p:t:-help" OPTION; do
        case $OPTION in
        
            p)  path=${OPTARG}
                ;;
            
            t) time=${OPTARG}
                ;;

            -help) Usage
                ;; 
        esac
    done

    if [[ $time -ge 0 ]]; then
        take_word $path $time
    else
        echo
    fi
fi