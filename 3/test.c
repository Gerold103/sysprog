#include "userfs.h"
#include "unit.h"
#include <limits.h>
#include <string.h>

static void
test_open(void)
{
	unit_test_start();
	
	int fd = ufs_open("file", 0);
	unit_check(fd == -1, "error when no such file");
	unit_check(ufs_errno() == UFS_ERR_NO_FILE, "errno is 'no_file'");

	fd = ufs_open("file", UFS_CREATE);
	unit_check(fd != -1, "use 'create' now");
	unit_check(ufs_close(fd) == 0, "close immediately");

	fd = ufs_open("file", 0);
	unit_check(fd != -1, "now open works without 'create'");
	unit_fail_if(ufs_close(fd) != 0);

	fd = ufs_open("file", UFS_CREATE);
	unit_check(fd != -1, "'create' is not an error when file exists");

	int fd2 = ufs_open("file", 0);
	unit_check(fd2 != -1, "open second descriptor");
	unit_check(fd2 != fd, "it is not the same in value");
	unit_check(ufs_close(fd2) == 0, "close the second");

	unit_check(ufs_close(fd) == 0, "and the first");

	unit_check(ufs_delete("file") == 0, "deletion");
	unit_check(ufs_open("file", 0) == -1, "now 'create' is needed again");

	unit_test_finish();
}

static void
test_stress_open(void)
{
	unit_test_start();

	const int count = 1000;
	int fd[count][2];
	char name[16], buf[16];
	unit_msg("open %d read and write descriptors, fill with data", count);
	for (int i = 0; i < count; ++i) {
		int name_len = sprintf(name, "file%d", i) + 1;
		int *in = &fd[i][0], *out = &fd[i][1];
		*in = ufs_open(name, UFS_CREATE);
		*out = ufs_open(name, 0);
		unit_fail_if(*in == -1 || *out == -1);
		ssize_t rc = ufs_write(*out, name, name_len);
		unit_fail_if(rc != name_len);
	}
	unit_msg("read the data back");
	for (int i = 0; i < count; ++i) {
		int name_len = sprintf(name, "file%d", i) + 1;
		int *in = &fd[i][0], *out = &fd[i][1];
		ssize_t rc = ufs_read(*in, buf, sizeof(buf));
		unit_fail_if(rc != name_len);
		unit_fail_if(memcmp(buf, name, rc) != 0);
		unit_fail_if(ufs_close(*in) != 0);
		unit_fail_if(ufs_close(*out) != 0);
		unit_fail_if(ufs_delete(name) != 0);
	}

	unit_test_finish();
}

static void
test_close(void)
{
	unit_test_start();

	unit_check(ufs_close(-1) == -1, "close invalid fd");
	unit_check(ufs_errno() == UFS_ERR_NO_FILE, "errno is set");

	unit_check(ufs_close(0) == -1, "close with seemingly normal fd");
	unit_fail_if(ufs_errno() != UFS_ERR_NO_FILE);

	unit_check(ufs_close(INT_MAX) == -1, "close with huge invalid fd");
	unit_fail_if(ufs_errno() != UFS_ERR_NO_FILE);

	int fd = ufs_open("file", UFS_CREATE);
	unit_fail_if(fd == -1);
	unit_check(ufs_close(fd) == 0, "close normal descriptor");
	unit_check(ufs_close(fd) == -1, "close it second time");
	unit_check(ufs_errno() == UFS_ERR_NO_FILE, "errno is set");

	unit_test_finish();
}

static void
test_io(void)
{
	unit_test_start();

	ssize_t rc = ufs_write(-1, NULL, 0);
	unit_check(rc == -1, "write into invalid fd");
	unit_check(ufs_errno() == UFS_ERR_NO_FILE, "errno is set");
	rc = ufs_write(0, NULL, 0);

	unit_check(rc == -1, "write into seemingly valid fd");
	unit_fail_if(ufs_errno() != UFS_ERR_NO_FILE);

	rc = ufs_read(-1, NULL, 0);
	unit_check(rc == -1, "read from invalid fd");
	unit_check(ufs_errno() == UFS_ERR_NO_FILE, "errno is set");
	rc = ufs_read(0, NULL, 0);

	unit_check(rc == -1, "read from seemingly valid fd");
	unit_fail_if(ufs_errno() != UFS_ERR_NO_FILE);

	int fd1 = ufs_open("file", UFS_CREATE);
	int fd2 = ufs_open("file", 0);
	unit_fail_if(fd1 == -1 || fd2 == -1);

	const char *data = "123";
	int size = strlen(data) + 1;
	unit_check(ufs_write(fd1, data, size) == size, "data is written");

	char buffer[2048];
	unit_check(ufs_read(fd2, buffer, sizeof(buffer)) == size,
		   "data is read");
	unit_check(memcmp(data, buffer, size) == 0, "the same data");

	ufs_close(fd1);
	ufs_close(fd2);
	ufs_delete("file");

	unit_test_finish();
}

static void
test_delete(void)
{
	unit_test_start();

	char c1, c2;
	int fd1 = ufs_open("file", UFS_CREATE);
	int fd2 = ufs_open("file", 0);
	int fd3 = ufs_open("file", 0);
	unit_fail_if(fd1 == -1 || fd2 == -1 || fd3 == -1);

	unit_check(ufs_delete("file") == 0,
		   "delete when opened descriptors exist");

	int tmp = ufs_open("tmp", UFS_CREATE);
	unit_fail_if(tmp == -1);
	unit_fail_if(ufs_write(tmp, "hhhhh", 5) != 5);
	ufs_close(tmp);

	unit_check(ufs_write(fd2, "a", 1) == 1,
		   "write into an fd opened before deletion");
	unit_check(ufs_read(fd3, &c1, 1) == 1,
		   "read from another opened fd - it sees the data");
	unit_check(c1 == 'a', "exactly the same data");
	unit_check(ufs_write(fd3, "bc", 2) == 2,
		   "write into it and the just read data is not overwritten");

	unit_check(ufs_read(fd1, &c1, 1) == 1, "read from the first one");
	unit_check(ufs_read(fd1, &c2, 1) == 1, "read from the first one again");
	unit_check(c1 == 'a' && c2 == 'b', "it reads data in correct order");

	int fd4 = ufs_open("file", 0);
	unit_check(fd4 == -1, "the existing 'ghost' file is not visible "\
		   "anymore for new opens");
	unit_check(ufs_errno() == UFS_ERR_NO_FILE, "errno is set");

	fd4 = ufs_open("file", UFS_CREATE);
	unit_fail_if(fd4 == -1);
	unit_check(ufs_read(fd4, &c1, 1) == 0,
		   "the file is created back, no data");
	unit_check(ufs_read(fd1, &c2, 1) == 1, "but the ghost still lives");
	unit_check(c2 == 'c', "and gives correct data");

	unit_check(ufs_delete("file") == 0, "delete it again");

	unit_fail_if(ufs_close(fd1) != 0);
	unit_fail_if(ufs_close(fd2) != 0);
	unit_fail_if(ufs_close(fd3) != 0);
	unit_fail_if(ufs_close(fd4) != 0);

	unit_fail_if(ufs_delete("tmp") != 0);

	unit_test_finish();
}

static void
test_max_file_size(void)
{
	unit_test_start();

	int fd = ufs_open("file", UFS_CREATE);
	unit_fail_if(fd == -1);

	int buf_size = 1024 * 1024;
	char *buf = (char *) malloc(buf_size);
	for (int i = 0; i < buf_size; ++i)
		buf[i] = 'a' + i % 26;
	for (int i = 0; i < 1024; ++i) {
		ssize_t rc = ufs_write(fd, buf, buf_size);
		unit_fail_if(rc != buf_size);
	}
	unit_check(ufs_write(fd, "a", 1) == -1,
		   "can not write over max file size");
	unit_check(ufs_errno() == UFS_ERR_NO_MEM, "errno is set");

	unit_fail_if(ufs_close(fd) != 0);
	fd = ufs_open("file", 0);
	unit_fail_if(fd == -1);
	char *buf2 = (char *) malloc(buf_size);
	for (int i = 0; i < 1014; ++i) {
		ssize_t rc = ufs_read(fd, buf2, buf_size);
		unit_fail_if(rc != buf_size);
		unit_fail_if(memcmp(buf2, buf, buf_size) != 0);
	}
	free(buf2);
	free(buf);
	unit_msg("read works");
	unit_fail_if(ufs_close(fd) == -1);
	unit_fail_if(ufs_delete("file") == -1);

	unit_test_finish();
}

static void
test_rights(void)
{
#ifdef NEED_OPEN_FLAGS
	unit_test_start();

	int fd = ufs_open("file", UFS_CREATE);
	unit_check(fd != -1, "file is opened with 'create' flag only");
	char *buf1 = "hello";
	int buf1_size = strlen(buf1) + 1;
	ssize_t rc = ufs_read(fd, buf1, buf1_size);
	unit_check(rc == 0, "it is allowed to read from it");
	rc = ufs_write(fd, buf1, buf1_size);
	unit_check(rc == buf1_size, "as well as write into it");
	unit_fail_if(ufs_close(fd) != 0);

	fd = ufs_open("file", 0);
	unit_check(fd != -1, "now opened without flags at all");
	char buf2[128];
	unit_check(ufs_read(fd, buf2, sizeof(buf2)) == buf1_size, "can read");
	unit_fail_if(memcmp(buf1, buf2, buf1_size) != 0);
	unit_check(ufs_write(fd, buf1, buf1_size) == buf1_size, "can write");
	unit_fail_if(ufs_close(fd) != 0);

	fd = ufs_open("file", UFS_READ_ONLY);
	unit_check(fd != -1, "opened with 'read only'");
	unit_check(ufs_read(fd, buf2, buf1_size) == buf1_size, "can read");
	unit_fail_if(memcmp(buf1, buf2, buf1_size) != 0);
	unit_check(ufs_write(fd, "bad", 4) == -1, "can not write");
	unit_check(ufs_errno() == UFS_ERR_NO_PERMISSION, "errno is set");
	unit_check(ufs_read(fd, buf2, sizeof(buf2)) == buf1_size,
		   "can again read");
	unit_check(memcmp(buf1, buf2, buf1_size) == 0,
		   "and data was not overwritten");
	unit_fail_if(ufs_close(fd) != 0);

	fd = ufs_open("file", UFS_WRITE_ONLY);
	unit_check(fd != -1, "opened with 'write only");
	unit_check(ufs_read(fd, buf2, sizeof(buf2)) == -1, "can not read");
	unit_check(ufs_errno() == UFS_ERR_NO_PERMISSION, "errno is set");
	char *buf3 = "new data which rewrites previous";
	int buf3_size = strlen(buf3) + 1;
	unit_check(ufs_write(fd, buf3, buf3_size) == buf3_size, "can write");
	unit_fail_if(ufs_close(fd));

	fd = ufs_open("file", UFS_READ_WRITE);
	unit_check(fd != -1, "opened with 'read write");
	unit_check(ufs_read(fd, buf2, sizeof(buf2)) == buf3_size, "can read");
	unit_check(memcmp(buf2, buf3, buf3_size) == 0, "data is correct");
	unit_check(ufs_write(fd, buf1, buf1_size) == buf1_size, "can write");
	unit_fail_if(ufs_close(fd));

	unit_fail_if(ufs_delete("file") != 0);
	unit_test_finish();
#endif
}

static void
test_resize(void)
{
#ifdef NEED_RESIZE
	unit_test_start();

	int fd = ufs_open("file", UFS_CREATE);
	unit_fail_if(fd == -1);
	char buffer[2048];
	memset(buffer, 'a', sizeof(buffer));
	ssize_t rc = ufs_write(fd, buffer, sizeof(buffer));
	unit_fail_if(rc != sizeof(buffer));
	int new_size = 23;
	rc = ufs_resize(fd, new_size);
	unit_check(rc == 0, "shrink to smaller size");

	int fd2 = ufs_open("file2", UFS_CREATE);
	unit_fail_if(fd2 == -1);
	rc = ufs_write(fd2, "123", 3);
	unit_fail_if(rc != 3);
	unit_fail_if(ufs_close(fd2) != 0);
	unit_fail_if(ufs_delete("file2") != 0);

	rc = ufs_write(fd, buffer, sizeof(buffer));
	unit_check(rc == sizeof(buffer),
		   "opened descriptor beyond new border still works");
	unit_fail_if(ufs_close(fd) != 0);
	unit_fail_if(ufs_delete("file") != 0);

	unit_test_finish();
#endif
}

int
main(void)
{
	unit_test_start();

	test_open();
	test_close();
	test_io();
	test_delete();
	test_stress_open();
	test_max_file_size();
	test_rights();
	test_resize();

	unit_test_finish();
	return 0;
}
