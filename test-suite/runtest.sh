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
	$CC tests/$TEST.c -L../../libgnomeui/.libs -lgnomeui -L../../libgnome/.libs \
		-lgnome -I../.. -lgtk -lgdk -lglib -ldl -L/usr/X11R6/lib \
		-lXext -lSM -lX11 -lm \
		-Wl,-rpath,../libgnome/.libs \
		-Wl,-rpath,../libgnomeui/.libs \
		-Wl,-rpath,../../libgnome/.libs \
		-Wl,-rpath,../../libgnomeui/.libs \
		-g \
		-o tests/$TEST > /tmp/.$$ 2>&1
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
