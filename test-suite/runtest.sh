#!/bin/sh

test -z "$srcdir" && srcdir=`dirname $0` && test -z "$srcdir" && srcdir=.

: ${COMPILE:="gcc -g -O2 -I.. -I$srcdir/.. `gtk-config --cflags` -c"}
: ${LINK:="gcc -g -O2 $LDFLAGS"}
: ${LDADD:="../libgnomeui/.libs/libgnomeui.a ../libgnome/.libs/libgnome.a `gtk-config --libs`"}

# $LINK passed in sometimes has a `-o check-TESTS'
LINK=`echo $LINK | sed -e 's,-o check-TESTS,,g'`

exec 3>./results.summary
if test -z "$*"; then
	TESTS="`find $srcdir/tests -name '*.c'`"
else
	TESTS="$@"
fi

mkdir results > /dev/null 2>&1 || true

export srcdir

for I in $TESTS; do
    TEST=`basename $I .c`
    if $COMPILE -c $srcdir/tests/$TEST.c -o $TEST.o > /tmp/.$$ 2>&1
    then : 
    else
	echo "CFAIL: $TEST" >&3
	echo "CFAIL: $TEST"
	cp /tmp/.$$ results/$TEST.out && rm -f /tmp/.$$
	failed=yes
	continue
    fi

    if $LINK $TEST.o $LDADD $LIBS -o test_$TEST >> /tmp/.$$ 2>&1
    then :
    else
	echo "CFAIL: $TEST" >&3
	echo "CFAIL: $TEST"
	cp /tmp/.$$ results/$TEST.out && rm -f /tmp/.$$
	failed=yes
	continue
    fi

    rm -f /tmp/.$$ $TEST.out

    ./test_$TEST > results/$TEST.out 2>&1
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
	rm -f $TEST.o test_$TEST
    else
	echo "FAIL: $TEST" >&3
	echo "FAIL: $TEST"
	failed=yes
    fi
done

test "yes" = "$failed" && exit 1
exit 0
