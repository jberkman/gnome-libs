#!/bin/sh

test -z "$srcdir" && srcdir=`dirname $0` && test -z "$srcdir" && srcdir=.
: ${COMPILE:="gcc -g -O2 -I.. -I$srcdir/.. `gtk-config --cflags` -c"}
: ${LINK:="gcc -g -O2 $LDFLAGS"}
: ${LDADD:="../libgnomeui/.libs/libgnomeui.a ../libgnome/.libs/libgnome.a `gtk-config --libs`"}

exec 3>./results.summary
if test -z "$*"; then
	TESTS="`find $srcdir/tests -name '*.c'`"
else
	TESTS="$@"
fi

mkdir tests || true > /dev/null 2>&1
mkdir results || true > /dev/null 2>&1

failed=no

for I in $TESTS; do
    TEST=`basename $I .c`
    $COMPILE $srcdir/tests/$TEST.c -o tests/$TEST.o > /tmp/.$$ 2>&1
    T=$?
    if test $T != 0; then
	echo "CFAIL: $TEST" >&3
	echo "CFAIL: $TEST"
	cp /tmp/.$$ results/$TEST.out && rm -f /tmp/.$$
	failed=yes
	continue
    fi

    $LINK tests/$TEST.o $LDADD $LIBS >> /tmp/.$$ 2>&1
    T=$?
    if test $T != 0; then
	echo "CFAIL: $TEST" >&3
	echo "CFAIL: $TEST"
	cp /tmp/.$$ results/$TEST.out && rm -f /tmp/.$$
	failed=yes
	continue
    fi

    rm -f /tmp/.$$
    tests/$TEST > results/$TEST.out 2>&1
    T=$?
    echo "Exit code $T" >> results/$TEST.out
    if test $T != 0; then
	echo "FAIL: $TEST" >&3
	echo "FAIL: $TEST"
	rm -f core
	failed=yes
	continue
    fi

    if diff -q $srcdir/expected/$TEST.out results/$TEST.out; then
	echo "PASS: $TEST" >&3
	echo "PASS: $TEST"
    else
	echo "FAIL: $TEST" >&3
	echo "FAIL: $TEST"
    fi
done

if test "$failed" = "yes"; then exit 1; fi
exit 0
