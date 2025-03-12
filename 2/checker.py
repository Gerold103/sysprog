import argparse
import os
import pathlib
import shutil
import subprocess
import sys
import test_parser

test_dir = './testdir'
small_timeout = 3
bonus_prefix_logic = 'bonus logical operators'
bonus_prefix_backround = 'bonus background'
bonus_prefix_all = 'bonus all'
points = 15

parser = argparse.ArgumentParser(description='Tests for shell')
parser.add_argument('-e', type=str, default='./a.out',
                    help='executable shell file')
parser.add_argument('--with_logic', type=bool, default=False,
                    help='Enable boolean logic bonus')
parser.add_argument('--with_background', type=bool, default=False,
                    help='Enable background bonus')
parser.add_argument('--many_args_count', type=int, default=100 * 1000,
                    help='Number of arguments for the extra-many-args test')
parser.add_argument('--tests', type=str, default='./tests.txt',
                    help='File with tests')
parser.add_argument('--verbose', type=bool, default=False,
                    help='Print more info for all sorts of things')
args = parser.parse_args()

if args.with_logic:
    points += 5
if args.with_background:
    points += 5

exe_path = os.path.abspath(args.e)
all_sections = test_parser.parse(args.tests)
test_sections = []
for section in all_sections:
    if section.type.startswith(bonus_prefix_all) and \
       (not args.with_logic or not args.with_background):
        print('Skipped on all bonuses section {}'.format(section.name))
        continue
    if section.type.startswith(bonus_prefix_logic) and not args.with_logic:
        print('Skipped on logic section {}'.format(section.name))
        continue
    if section.type.startswith(bonus_prefix_backround) and not args.with_background:
        print('Skipped on background section {}'.format(section.name))
        continue
    test_sections.append(section)

def open_new_shell():
    return subprocess.Popen([exe_path], shell=False, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, bufsize=0,
                            cwd=test_dir)

def cleanup():
    shutil.rmtree(test_dir, ignore_errors=True)

def recreate_dir():
    cleanup()
    pathlib.Path(test_dir).mkdir(exist_ok=True)

def print_diff(expected, got):
    with open('output_expected.txt', 'w') as f:
        f.write(expected)
    with open('output_got.txt', 'w') as f:
        f.write(got)
    print('<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Diff >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>')
    print(subprocess.run(
        ['diff -y output_expected.txt output_got.txt'],
        shell=True,
        stdout=subprocess.PIPE).stdout.decode('utf-8'))

##########################################################################################
print('⏳ Running tests one by one')
recreate_dir()
for section in test_sections:
    for case in section.cases:
        p = open_new_shell()
        try:
            output_got = p.communicate(case.body.encode(), small_timeout)[0].decode()
        except subprocess.TimeoutExpired:
            print('Too long no output for test {} on line {}. '\
                  'Probably you forgot to process EOF or is '\
                  'stuck in wait/waitpid()'.format(case.name, case.line))
            sys.exit(-1)
        if output_got != case.output:
            print('Test output mismatch for {} on line {}'.format(case.name, case.line))
            if args.verbose:
                print_diff(case.output, output_got)
            sys.exit(-1)
print('✅ Passed')

##########################################################################################
print('⏳ Running tests in one shell')
recreate_dir()
p = open_new_shell()
input_cmd = ''
output_exp = ''
for section in test_sections:
    for case in section.cases:
        input_cmd += case.body
        output_exp += case.output
try:
    output_got = p.communicate(input_cmd.encode(), small_timeout)[0].decode()
except subprocess.TimeoutExpired:
    print('Too long no output. Probably you forgot to process EOF or is stuck '\
          'in wait/waitpid()')
    sys.exit(-1)
if p.returncode != 0:
    print('Expected zero exit code')
    sys.exit(-1)
if output_got != output_exp:
    print('Tests output mismatch')
    if args.verbose:
        print_diff(output_exp, output_got)
    sys.exit(-1)
print('✅ Passed')

##########################################################################################
# Explicitly test the 'exit' command. It is expected to terminate the shell,
# hence tested separately.
print('⏳ Test \'exit\' command')
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
        print('Too long no output. Probably you forgot to handle "exit" manually')
        sys.exit(-1)
    p.terminate()
    if p.returncode != test[1]:
        print('Wrong exit code in test "{}"'.format(test[0]))
        print('Expected {}, got {}'.format(test[1], p.returncode))
        sys.exit(-1)
print('✅ Passed')

##########################################################################################
# Exit code should be from the last used command.
print('⏳ Test exit code after or before certain commands')
tests = [
(["ls /"], 0),
(["ls / | exit 123"], 123),
(["ls /404", "echo test"], 0),
]
cmd = "ls /404"
code = os.WEXITSTATUS(os.system(cmd + ' 2>/dev/null'))
assert(code != 0)
tests.append(([cmd], code))

if args.with_logic:
    tests.append((["exit 123 && echo test"], 123))
    tests.append((["exit 123 || echo test"], 123))

for test in tests:
    p = open_new_shell()
    try:
        for cmd in test[0]:
            p.stdin.write(cmd.encode() + b'\n')
        p.stdin.close()
        p.wait(1)
    except subprocess.TimeoutExpired:
        print('Too long no output. Probably you forgot to handle "exit" manually')
        sys.exit(-1)
    p.terminate()
    if p.returncode != test[1]:
        print('Wrong exit code in test "{}"'.format(test[0]))
        print('Expected {}, got {}'.format(test[1], p.returncode))
        sys.exit(-1)
print('✅ Passed')

##########################################################################################
# Test an extra long command. To ensure the shell doesn't have an internal
# buffer size limit (well, it always can allocate like 1GB, but this has to be
# caught at review).
count = 100 * 1024
print('⏳ Test an extra long command ({} symbols)'.format(count))
p = open_new_shell()
output_expected = 'a' * count + '\n'
command = 'echo ' + output_expected
is_error = False
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
    sys.exit(-1)
print('✅ Passed')

##########################################################################################
# Test extra many args. To ensure the shell doesn't have an internal argument
# count limit (insane limits like 1 million have to be caught at review).
count = args.many_args_count
print('⏳ Test extra many arguments ({} count)'.format(count))
p = open_new_shell()
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
    sys.exit(-1)
print('✅ Passed')

##########################################################################################
# Test an extra long pipe. To ensure that the shell doesn't have a limit on the
# command count inside one line.
count = 1000
print('⏳ Test an extra long pipe ({} commands)'.format(count))
p = open_new_shell()
output_expected = 'test'
command = 'echo ' + output_expected + ' | cat' * count + '\n'
output_expected += '\n'
try:
    output = p.communicate(command.encode(), 5)[0].decode()
except subprocess.TimeoutExpired:
    print('Too long no output on an extra long pipe')
    is_error = True
p.terminate()
if not is_error and output != output_expected:
    print('Bad output for a command with an extra long pipe')
    is_error = True
if not is_error and p.returncode != 0:
    print('Bad return code for a command with an extra long pipe - '\
          'expected 0 (success)')
    is_error = True
if is_error:
    print('Failed a command with an extra long pipe '\
          '(`echo test | cat | cat | cat ... | cat` with '\
          '`cat` repeated {} times'.format(count))
    sys.exit(-1)

print('✅ Passed')
print('⏫ Points: {}'.format(points))
cleanup()
