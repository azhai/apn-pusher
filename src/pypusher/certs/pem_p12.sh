#!/bin/bash

BIN="/usr/bin/openssl"

convert_cert() {
    INFILE="$1"
    OUTFILE="${INFILE%%.*}.p12"
    if [ ! -f "$OUTFILE" ]; then
        $BIN pkcs12 -export -out $OUTFILE -in $INFILE
    fi
}

convert_cert "$1"
