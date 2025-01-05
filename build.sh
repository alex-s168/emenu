set -e

build() {
    if [ -z $CC ]; then
        if hash clang 2>/dev/null; then 
            : "${CC:=clang}"
        elif hash cc 2>/dev/null; then
            : "${CC:=cc}"
        elif hash gcc 2>/dev/null; then
            : "${CC:=gcc}"
        elif hash tcc 2>/dev/null; then
            : "${CC:=tcc}"
        else 
            >&2 echo no c compiler found 
            >&2 echo manually set CC!
            exit 1
        fi
    fi

    : "${CFLAGS:=}"
    : "${LDFLAGS:=}"

    "$CC" "$CFLAGS" "$LDFLAGS" -o emenu_path sortpath.c $(slowdb/build.sh)
}

if [ "$1" = "build" ]; then
    build
elif [ "$1" = "install" ]; then
    build

    : "${PREFIX:=/usr}"

    cp -f emenu_path emenu_run $DESTDIR$PREFIX/bin
    chmod 755 $DESTDIR$PREFIX/bin/emenu_path
    chmod 755 $DESTDIR$PREFIX/bin/emenu_run
else
    >&2 echo "Usage: ./build.sh [build|install]"
    exit 1
fi
