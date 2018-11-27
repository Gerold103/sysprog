touch test.txt
ls -l
# User with id 2000 shall not exist.
chown 2000 test.txt
ls -l
