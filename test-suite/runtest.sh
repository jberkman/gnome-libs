#!/bin/sh
CC=cc
exec 3>./results.summary
if [ -z "$@" ]; then
	TESTS=`find tests -name '*.c'`
else
	TESTS="$@"
fi
for I in $TESTS; do
	TEST=`basename $I .c`
	$CC -L../../libgnome/_libs -lgnome -L../../libgnomeui/_libs \
		-lgnomeui -I../.. -lgtk -lgdk -lglib -L/usr/X11R6/lib \
		-lXext -lSM -lX11 -lm \
		-Wl,-rpath,../../libgnome/_libs \
		-Wl,-rpath,../../libgnomeui/_libs \
		-Wl,-rpath,../../../libgnome/_libs \
		-Wl,-rpath,../../../libgnomeui/_libs \
		-g \
		-o tests/$TEST tests/$TEST.c > /tmp/.$$ 2>&1
	T=$?
	if [ $T != 0 ]; then
		echo "CFAIL: $TEST" >&3
		echo "CFAIL: $TEST"
		mv /tmp/.$$ results/$TEST.out
	else
		rm -f /tmp/.$$
		tests/$TEST > results/$TEST.out 2>&1
		T=$?
		echo "Exit code $T" >> results/$TEST.out
		if [ $T != 0 ]; then
			echo "FAIL: $TEST" >&3
			echo "FAIL: $TEST"
			rm -f core
		else
			if diff -q expected/$TEST.out results/$TEST.out; then
				echo "PASS: $TEST" >&3
				echo "PASS: $TEST"
			else
				echo "FAIL: $TEST" >&3
				echo "FAIL: $TEST"
			fi
		fi
	fi
done
