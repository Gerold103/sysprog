#!/bin/bash

# Build all *.c files. And then + leaks.

hw_number=$(jq -r '.hw_number' "$RESOURCES_DIR_MOUNT/hw_number.json") 

unzip -o $SOLUTION_MOUNT/*.zip -d $SOLUTION_MOUNT/

if [ "$hw_number" -eq 1 ]; then
	# In Work ...
	echo ""
elif [ "$hw_number" -eq 2 ]; then
    cp $RESOURCES_DIR_MOUNT/checker.py $SOLUTION_MOUNT
	cp $RESOURCES_DIR_MOUNT/Makefile $SOLUTION_MOUNT
	cd $SOLUTION_MOUNT
	make
	if python3 $SOLUTION_MOUNT/checker.py --max 25; then
		status="OK"
		score="25"
	elif python3 $SOLUTION_MOUNT/checker.py --max 20; then
		status="OK"
		score="20"
	elif python3 $SOLUTION_MOUNT/checker.py --max 15; then
		status="OK"
		score="15"
	else 
		status="FAIL"
		score="0"
	fi
elif [ "$hw_number" -eq 3 ] || [ "$hw_number" -eq 4 ] || [ "$hw_number" -eq 5 ]; then
	# In work ...
	cp $RESOURCES_DIR_MOUNT/test.c $SOLUTION_MOUNT
	cp $RESOURCES_DIR_MOUNT/Makefile $SOLUTION_MOUNT
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