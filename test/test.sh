#!/bin/sh
cd /Users/rogers/src/sprotoc/test/test
TESTS="test1 test_b"
# end of test.sh (running the tests)

fail=""
for t in $TESTS; do
    echo " ==================== $t ===================="
    ./$t || {
        echo "$t returned $?"
        fail="$fail $t"
    }
    echo
done
echo " ==================== Summary: ===================="

if [[ x"" == x"$fail" ]]; then
    echo "All tests passed!"
else
    echo "Failed tests: $fail"
    exit 0
fi

