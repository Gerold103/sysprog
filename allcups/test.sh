#!/bin/bash

if [ "$IS_LOCAL" == "1" ]; then
	hw=$HW
	SOLUTION_MOUNT="$RESOURCES_DIR_MOUNT/$hw"
	cp $SOLUTION_MOUNT/* /sysprog/solution
else
	unzip -o $SOLUTION_MOUNT -d /sysprog/solution
	hw=$(jq -r '.hw' "$RESOURCES_DIR_MOUNT/allcups/settings.json")
fi

cp -r $RESOURCES_DIR_MOUNT/* /sysprog

status='"ERR"'
score=0

if [ "$hw" -eq 2 ]; then
	cp $RESOURCES_DIR_MOUNT/2/checker.py /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/2/Makefile /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/2/tests.txt /sysprog/solution
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
	echo $output
else
	result=$(cat << EOF
{
	"status": $status,
	"validationQuality": $score,
	"testingQuality": $score,
	"errors": $errors_json
}
EOF
	)
	echo "$result"
	echo $result > $RESULT_LOCATION
fi
