# this is a -*- sh -*- script fragment
# source this file with $srcdir and $the_test set.
# $the_test is the name of the desired test

export srcdir

mkdir results > /dev/null 2>&1 || true

./tests/$the_test > results/$the_test.out 2>&1
T=$?
echo "Exit code $T" >> results/$the_test.out

# magic exit code 77 (EX_NOPERM: permission denied)
if test $T = 77; then exit 77; fi

if test $T = 0; then
  if diff -q $srcdir/expected/$the_test.out results/$the_test.out; then
     exit 0
  fi
fi

exit 1
