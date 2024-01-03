#!/usr/bin/env bash
cc good.c -o good -s
cc bad.c -o bad -s
cp good bad ..
rm good bad