#!/bin/bash

rm -rf tests/*.checked

ALL=0
for i in tests/*.in; do
    CHECKED=$(echo "$i" | sed s/[.]in/.checked/)
    ./parser "$(head -n 1 $i)" > "$CHECKED"
    ALL=$((ALL+1))
done

echo "$ALL tests updated"