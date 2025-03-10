#!/bin/bash

cp -r $RESOURCES_DIR_MOUNT/* /sysprog

if [ "$IS_LOCAL" == "1" ]; then
	hw=$HW
	SOLUTION_MOUNT="$RESOURCES_DIR_MOUNT/$hw"
	cp /sysprog/"$hw"/* /sysprog/solution
	cp $SOLUTION_MOUNT/* /sysprog/solution
else
	hw=$(jq -r '.hw' "$RESOURCES_DIR_MOUNT/allcups/settings.json")
	cp /sysprog/"$hw"/* /sysprog/solution
	unzip -o $SOLUTION_MOUNT -d /sysprog/solution
fi

status='"ERR"'
score=0
output=''

if [ "$hw" -eq 1 ]; then
	cp $RESOURCES_DIR_MOUNT/"$hw"/test.c /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/"$hw"/Makefile /sysprog/solution
	rm -f /sysprog/solution/libcoro_test.c
	cd /sysprog/solution

	echo '🔨 Building'
	make test_glob

	echo '⏳ Running tests'
	if output=$(./test 2>&1); then
		status='"OK"'
		score=$(./test --max_points)
	fi
elif [ "$hw" -eq 2 ]; then
	cp $RESOURCES_DIR_MOUNT/"$hw"/checker.py /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/"$hw"/Makefile /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/"$hw"/tests.txt /sysprog/solution
	rm /sysprog/solution/parser_test.c
	cd /sysprog/solution

	echo '🔨 Building'
	make test_glob

	echo '⏳ Running tests'
	if output=$(python3 checker.py --with_logic 1 --with_background 1 -e ./mybash 2>&1); then
		status='"OK"'
		score="25"
	elif output=$(python3 checker.py --with_logic 1 -e ./mybash 2>&1); then
		status='"OK"'
		score="20"
	elif output=$(python3 checker.py --with_background 1 -e ./mybash 2>&1); then
		status='"OK"'
		score="20"
	elif output=$(python3 checker.py -e ./mybash 2>&1); then
		status='"OK"'
		score="15"
	fi
elif [ "$hw" -eq 3 ] || [ "$hw" -eq 4 ] || [ "$hw" -eq 5 ]; then
	cp $RESOURCES_DIR_MOUNT/"$hw"/test.c /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/"$hw"/Makefile /sysprog/solution
	rm -f /sysprog/solution/*_exe.c
	cd /sysprog/solution

	echo '🔨 Building'
	make test_glob

	echo '⏳ Running tests'
	if output=$(./test 2>&1); then
		status='"OK"'
		score=$(./test --max_points)
	fi
else
	echo "Unknown or unsupported HW number"
fi

errors_json=$(echo "$output" | jq -R -s '.')

echo '⚖️ Result:'
if [ "$IS_LOCAL" == "1" ]; then
	echo "Status: $status, points: $score"
	echo "$output"
else
	result=$(cat << EOF
{
	"status": $status,
	"validationQuality": $score,
	"testingQuality": $score,
	"errors": [$errors_json]
}
EOF
	)
	echo "$result"
	echo $result > $RESULT_LOCATION
fi
