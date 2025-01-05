set -e
if [ -z $1 ]; then
    echo expected argument: version
    exit 1
fi
mkdir emenu-$1
files="$(git ls-files) $(cd slowdb/ && git ls-files | awk '{print "slowdb/"$1}')"
for f in $files; do
    if [ -f $f ]; then
        if ! [ -z $(dirname "$f") ]; then
            mkdir -p emenu-$1/$(dirname "$f")
        fi
        cp $f emenu-$1/$(dirname "$f")/
    fi
done
tar -czf emenu-$1.tar.gz emenu-$1
rm -r emenu-$1
