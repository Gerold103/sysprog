import random
import argparse

maxint = 1 << 31

parser = argparse.ArgumentParser(description = "Generate random numbers file")
parser.add_argument('-f', type=str, required=True, help="file name")
parser.add_argument('-c', type=int, required=True, help='number count')
parser.add_argument('-m', type=int, default=maxint, help='maximal number')
args = parser.parse_args()
random.seed()


f = open(args.f, 'w')

for i in range(0, args.c):
	f.write(str(random.randint(0, args.m)))
	if i + 1 != args.c:
		f.write(' ')

f.close()
