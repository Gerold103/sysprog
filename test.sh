#!/bin/bash

# Build all *.c files. And then + leaks.

hw=$(jq -r '.hw' "$RESOURCES_DIR_MOUNT/settings.json") 

cp -r $RESOURCES_DIR_MOUNT/* /sysprog

if [ "$NOZIP" == "1" ]; then 
	cp $SOLUTION_MOUNT/* /sysprog/solution
else
	unzip -o $SOLUTION_MOUNT -d /sysprog/solution
fi

if [ "$hw" -eq 1 ]; then
	# In Work ...
	echo ""
elif [ "$hw" -eq 2 ]; then
    cp $RESOURCES_DIR_MOUNT/2/checker.py /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/2/Makefile /sysprog/solution
	cd /sysprog/solution

	make test_glob
	
	if python3 checker.py --with_logic 1 --with_background 1 -e ./mybash; then
		status='"OK"'
		score="25"
	elif python3 checker.py --with_logic 1 -e ./mybash; then
		status='"OK"'
		score="20"
	elif python3 checker.py --with_background 1 -e ./mybash; then
		status='"OK"'
		score="20"
	elif python3 checker.py -e ./mybash --verbose 1; then
		status='"OK'
		score="15"
	else 
		status='"ERR"'
		score="0"
	fi
elif [ "$hw" -eq 3 ] || [ "$hw" -eq 4 ] || [ "$hw" -eq 5 ]; then
	cp $RESOURCES_DIR_MOUNT/"$hw"/test.c /sysprog/solution
	cp $RESOURCES_DIR_MOUNT/"$hw"/Makefile /sysprog/solution
	cd /sysprog/solution
	
	make test_glob

	if ./test; then
		status='"OK"'
		score=$(./test --max_points)
	else
		status='"ERR"'
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