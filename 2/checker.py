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

p = subprocess.Popen([args.e], shell=False, stdin=subprocess.PIPE,
		     stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
		     bufsize=0)

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
"cat \"my file with whitespaces in name.txt\""
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
],
[
"false && echo 123",
"true && echo 123",
"true || false && echo 123",
"true || false || true && echo 123",
"false || echo 123"
],
[
"sleep 0.5 && echo 'back sleep is done' &",
"echo 'next sleep is done'",
"sleep 0.5",
]
]

def finish(code):
	os.system('rm -rf testdir')
	sys.exit(code)

prefix = '--------------------------------'
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

try:
	output = p.communicate(command.encode(), 3)[0].decode()
except subprocess.TimeoutExpired:
	print('Too long no output. Probably you forgot to process EOF')
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
	print('{}\nThe tests did not pass'.format(prefix))
	finish(-1)
else:
	print('{}\nThe tests passed'.format(prefix))
	finish(0)
