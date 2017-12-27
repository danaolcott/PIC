#!/bin/bash
######################################
#12/24/17
#Dana Olcott
#
#build script for sdcc

#name of input file as arg, if no arg,
#defaults to main.c
NUM=$#
INPUT_FILE="main.c"
TARGET="output"

if test "$NUM" -eq "1";
then
    INPUT_FILE=$1
fi

echo "Input file: $INPUT_FILE"

#clean - remove all files that begin with target
find . -type f -name "$TARGET*" -exec rm {} \;

######################################################
#commandline for sdcc
#include path
PROCESSOR="16f690"
PORT="pic14"
DEVICE="__16f690"

CC_OPTIONS="-p$PROCESSOR -m$PORT -V --verbose --std-sdcc99 --use-non-free --debug -o $TARGET"



DEFS="-D$DEVICE"

I_PATH1="/usr/local/share/sdcc/include"
I_PATH2="/usr/local/share/sdcc/non-free/include/pic14"
I_PATH3="/usr/local/share/sdcc/lib/pic14"
I_PATH4="/usr/local/share/sdcc/non-free/lib/pic14"
I_PATH5="/usr/share/gputils/lkr/"

I_PATH="-I$I_PATH1 -I$I_PATH2 -I$I_PATH3 -I$I_PATH4 -I$I_PATH5" 


sdcc $CC_OPTIONS $DEFS $INPUT_FILE $I_PATH

#this works too...
#sdcc -p16f690 -mpic14 -V --verbose --std-sdcc99 --use-non-free --debug $DEFS -o output $INPUT_FILE $I_PATH 




