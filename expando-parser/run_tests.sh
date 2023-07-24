#!/bin/bash

ALL=0
SUCC=0

rm -rf tests/*.out

for i in tests/*.in; do
    OUT=$(echo "$i" | sed s/[.]in/.out/)
    CKECKED=$(echo "$i" | sed s/[.]in/.checked/)
    ./parser "$(head -n 1 $i)" > "$OUT"
    DIFF="$(diff $CKECKED $OUT | wc -l)"
    if [ -f $CKECKED ] && [ "$DIFF" = "0" ] 
    then
        SUCC=$((SUCC+1))
    else
        echo "$i failed"
    fi

    ALL=$((ALL+1))
done

echo "[$SUCC/$ALL] tests succeded"