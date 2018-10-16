#include <stdlib.h>

struct complex_struct {
	int id;
	double a;
	long d;
	char buf[10];
	char *long_buf;
};

struct complex_struct *
complex_struct_bad_new(int long_buf_len)
{
	struct complex_struct *ret =
		(struct complex_struct *) malloc(sizeof(*ret));
	ret->long_buf = (char *) malloc(long_buf_len);
	return ret;
}

struct complex_struct *
complex_struct_good_new(int long_buf_len)
{
	struct complex_struct *ret;
	int size = sizeof(*ret) + long_buf_len;
	ret = (struct complex_struct *) malloc(size);
	ret->long_buf = (char *) ret + sizeof(*ret);
	return ret;
}

int main()
{
	return 0;
}
