#!/bin/bash

set -e

CMD="${1:-check}"

FLAGS=""

case $CMD in
    apply)
        FLAGS="-i" # run clang-format with inplace fixing enabled
        ;;
    check)
        FLAGS="--dry-run -Werror" # run clang-format as verifier
        ;;
    *)
        echo "Unknown command: ${CMD}"
        exit 1
        ;;
esac

DIRS="src/"

for source in $(find ${DIRS} -name \*.cpp -o -name \*.hpp -o -name \*.cc -o -name \*.hh -o -name \*.c -o -name \*.h)
do
    if [ "${source}" != "src/Xdmac/component_xdmac.h" ] # external file integrated into lib
    then
        clang-format ${FLAGS} "${source}"
    fi
done
