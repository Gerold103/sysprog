#!/bin/bash

# Build all *.c files. And then + leaks.

hw=$(jq -r '.hw' "$RESOURCES_DIR_MOUNT/settings.json") 

unzip -o $SOLUTION_MOUNT -d /sysprog

if [ "$hw" -eq 1 ]; then
	# In Work ...
	echo ""
elif [ "$hw" -eq 2 ]; then
    cp $RESOURCES_DIR_MOUNT/checker.py /sysprog
	cp $RESOURCES_DIR_MOUNT/Makefile /sysprog
	cd /sysprog
	make
	if python3 /sysprog/checker.py --max 25; then
		status="OK"
		score="25"
	elif python3 /sysprog/checker.py --max 20; then
		status="OK"
		score="20"
	elif python3 /sysprog/checker.py --max 15; then
		status="OK"
		score="15"
	else 
		status="FAIL"
		score="0"
	fi
elif [ "$hw" -eq 3 ] || [ "$hw" -eq 4 ] || [ "$hw" -eq 5 ]; then
	# In work ...
	cp $RESOURCES_DIR_MOUNT/test.c /sysprog
	cp $RESOURCES_DIR_MOUNT/Makefile /sysprog
	cd $SOLUTION_MOUNT
	make
	if ./test; then
		status="OK"
		score="15"
	else
		status="FAIL"
		score=0
	fi
fi

	cat << EOF > $RESULT_LOCATION
{
	"status": $status,
	"validationQuality": $score,
	"testingQuality": $score
}
EOF