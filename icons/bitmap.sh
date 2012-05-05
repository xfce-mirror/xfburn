#!/bin/sh

cd "`dirname $0`"

if [ ! -x "`which inkscape`" ]; then
  echo "Could not find inkscape"
  exit 1
fi

convert() {
    in=$1
    w=$2
    h=$3
    out=${w}x${h}/xfburn.png

    echo "Converting $in to $out"

    inkscape -z -f $in -e $out -w $w -h $h
}

convert xfburn-16.svg 16 16
convert xfburn-48.svg 22 22
convert xfburn-48.svg 24 24
convert xfburn-48.svg 32 32
convert xfburn-48.svg 48 48
cp xfburn-128.svg scalable/xfburn.svg
