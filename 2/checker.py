import subprocess
import argparse
import sys
import os

parser = argparse.ArgumentParser(description='Tests for shell')
parser.add_argument('-e', type=str, default='./a.out',
	            help='executable shell file')
parser.add_argument('-r', type=str, help='result file')
parser.add_argument('-t', action='store_true', default=False,
		    help='run without checks')
parser.add_argument('--max', type=int, choices=[15, 20, 25], default=15,
		    help='max points number')
args = parser.parse_args()

tests = [
[
"mkdir testdir",
"cd testdir",
'pwd | tail -c 8',
'   pwd | tail -c 8',
],
[
"touch \"my file with whitespaces in name.txt\"",
"ls",
"echo '123 456 \" str \"'",
"echo '123 456 \" str \"' > \"my file with whitespaces in name.txt\"",
"cat my\\ file\\ with\\ whitespaces\\ in\\ name.txt",
"echo \"test\" >> \"my file with whitespaces in name.txt\"",
"cat \"my file with whitespaces in name.txt\"",
"echo 'truncate' > \"my file with whitespaces in name.txt\"",
"cat \"my file with whitespaces in name.txt\"",
"echo \"test 'test'' \\\\\" >> \"my file with whitespaces in name.txt\"",
"cat \"my file with whitespaces in name.txt\"",
"echo \"4\">file",
"cat file",
"echo 100|grep 100",
],
[
"# Comment",
"echo 123\\\n456",
],
[
"rm my\\ file\\ with\\ whitespaces\\ in\\ name.txt",
"echo 123 | grep 2",
"echo 123\\\n456\\\n| grep 2",
"echo \"123\n456\n7\n\" | grep 4",
"echo 'source string' | sed 's/source/destination/g'",
"echo 'source string' | sed 's/source/destination/g' | sed 's/string/value/g'",
"echo 'source string' |\\\nsed 's/source/destination/g'\\\n| sed 's/string/value/g'",
"echo 'test' | exit 123 | grep 'test2'",
"echo 'source string' | sed 's/source/destination/g' | sed 's/string/value/g' > result.txt",
"cat result.txt",
"yes bigdata | head -n 100000 | wc -l | tr -d [:blank:]",
"exit 123 | echo 100",
"echo 100 | exit 123",
"printf \"import time\\n\\\n"\
	"time.sleep(0.1)\\n\\\n"\
	"f = open('test.txt', 'w')\\n\\\n"\
	"f.write('Text\\\\\\n')\\n\\\n"\
	"f.close()\\n\" > test.py",
"python test.py | exit 0",
"cat test.txt",
],
[
"false && echo 123",
"true && echo 123",
"true || false && echo 123",
"true || false || true && echo 123",
"false || echo 123",
"echo 100 || echo 200",
"echo 100 && echo 200",
"echo 100 | grep 1 || echo 200 | grep 2",
"echo 100 | grep 1 && echo 200 | grep 2",
],
[
"sleep 0.5 && echo 'back sleep is done' &",
"echo 'next sleep is done'",
"sleep 0.5",
]
]

prefix = '--------------------------------'

def finish(code):
	os.system('rm -rf testdir')
	sys.exit(code)

def open_new_shell():
	return subprocess.Popen([args.e], shell=False, stdin=subprocess.PIPE,
				stdout=subprocess.PIPE,
				stderr=subprocess.STDOUT, bufsize=0)

def exit_failure():
	print('{}\nThe tests did not pass'.format(prefix))
	finish(-1)

command = ''
for section_i, section in enumerate(tests, 1):
	if section_i == 5 and args.max == 15:
		break
	if section_i == 6 and args.max != 25:
		break
	command += 'echo "{}Section {}"\n'.format(prefix, section_i)
	for test_i, test in enumerate(section, 1):
		command += 'echo "$> Test {}"\n'.format(test_i)
		command += '{}\n'.format(test)

p = open_new_shell()
try:
	output = p.communicate(command.encode(), 3)[0].decode()
except subprocess.TimeoutExpired:
	print('Too long no output. Probably you forgot to process EOF')
	finish(-1)
if p.returncode != 0:
	print('Expected zero exit code')
	finish(-1)

if args.t:
	print(output)
	finish(0)

output = output.splitlines()
result = args.r
if not result:
	result = 'result{}.txt'.format(args.max)
with open(result) as f:
	etalon = f.read().splitlines()
etalon_len = len(etalon)
output_len = len(output)
line_count = min(etalon_len, output_len)
is_error = False
for i in range(line_count):
	print(output[i])
	if output[i] != etalon[i]:
		print('Error in line {}. '\
		      'Expected:\n{}'.format(i + 1, etalon[i]))
		is_error = True
		break
if not is_error and etalon_len != output_len:
	print('Different line count. Got {}, '\
	      'expected {}'.format(output_len, etalon_len))
	is_error = True
p.terminate()
if is_error:
	exit_failure()

# Explicitly test the 'exit' command. It is expected to terminate the shell,
# hence tested separately.
tests = [
("exit", 0),
("  exit ", 0),
("  exit   10  ", 10),
]
for test in tests:
	p = open_new_shell()
	try:
		p.stdin.write(test[0].encode() + b'\n')
		p.wait(1)
	except subprocess.TimeoutExpired:
		print('Too long no output. Probably you forgot to '\
		      'handle "exit" manually')
		finish(-1)
	p.terminate()
	if p.returncode != test[1]:
		print('Wrong exit code in test "{}"'.format(test[0]))
		print('Expected {}, got {}'.format(test[1], p.returncode))
		exit_failure()

# Exit code should be from the last used command.
tests = [
(["ls /"], 0),
(["ls / | exit 123"], 123),
(["ls /404", "echo test"], 0),
]
cmd = "ls /404"
code = os.WEXITSTATUS(os.system(cmd + ' 2>/dev/null'))
assert(code != 0)
tests.append(([cmd], code))

for test in tests:
	p = open_new_shell()
	try:
		for cmd in test[0]:
			p.stdin.write(cmd.encode() + b'\n')
		p.stdin.close()
		p.wait(1)
	except subprocess.TimeoutExpired:
		print('Too long no output. Probably you forgot to '\
		      'handle "exit" manually')
		finish(-1)
	p.terminate()
	if p.returncode != test[1]:
		print('Wrong exit code in test "{}"'.format(test[0]))
		print('Expected {}, got {}'.format(test[1], p.returncode))
		exit_failure()

# Test an extra long command. To ensure the shell doesn't have an internal
# buffer size limit (well, it always can allocate like 1GB, but this has to be
# caught at review).
p = open_new_shell()
count = 100 * 1024
output_expected = 'a' * count + '\n'
command = 'echo ' + output_expected
try:
	output = p.communicate(command.encode(), 5)[0].decode()
except subprocess.TimeoutExpired:
	print('Too long no output on an extra big command')
	is_error = True
p.terminate()
if not is_error and output != output_expected:
	print('Bad output for an extra big command')
	is_error = True
if not is_error and p.returncode != 0:
	print('Bad return code for an extra big command - expected 0 (success)')
	is_error = True
if is_error:
	print('Failed an extra big command (`echo a....` with `a` '\
	      'repeated {} times'.format(count))
	exit_failure()

# Test extra many args. To ensure the shell doesn't have an internal argument
# count limit (insane limits like 1 million have to be caught at review).
p = open_new_shell()
count = 100 * 1000
output_expected = 'a ' * (count - 1) + 'a\n'
command = 'echo ' + output_expected
try:
	output = p.communicate(command.encode(), 5)[0].decode()
except subprocess.TimeoutExpired:
	print('Too long no output on a command with extra many args')
	is_error = True
p.terminate()
if not is_error and output != output_expected:
	print('Bad output for a command with extra many args')
	is_error = True
if not is_error and p.returncode != 0:
	print('Bad return code for a command with extra many args - '\
	      'expected 0 (success)')
	is_error = True
if is_error:
	print('Failed a command with extra many args (`echo a a a ...` with '\
	      '`a` repeated {} times'.format(count))
	exit_failure()

print('{}\nThe tests passed'.format(prefix))
finish(0)
