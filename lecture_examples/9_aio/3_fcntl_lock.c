#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

int
write_symbols(int fd, char *cmd)
{
	int n;
	char c;
	sscanf(cmd, "%d %c", &n, &c);
	for (int i = 0; i < n; ++i) {
		if (write(fd, &c, sizeof(c)) == -1)
			return -1;
	}
	return 0;
}

int
char_to_whence(char whence)
{
	if (whence == 's')
		return SEEK_SET;
	if (whence == 'e')
		return SEEK_END;
	return SEEK_CUR;
}

int
char_to_type(char type)
{
	if (type == 'r')
		return F_RDLCK;
	if (type == 'w')
		return F_WRLCK;
	return F_UNLCK;
}

int
do_lock(int fd, bool block, char *cmd)
{
	char whence, type;
	int start, len;
	sscanf(cmd, "%c %c %d %d", &type, &whence, &start, &len);
	printf("type = %c, whence = %c, start = %d, len = %d\n", type, whence,
	       start, len);
	struct flock fl;
	fl.l_type = char_to_type(type);
	fl.l_whence = char_to_whence(whence);
	fl.l_start = start;
	fl.l_len = len;
	return fcntl(fd, block ? F_SETLKW : F_SETLK, &fl);
}

int
get_lock(int fd, char *cmd)
{
	char whence, type;
	int start, len;
	sscanf(cmd, "%c %c %d %d", &type, &whence, &start, &len);
	printf("type = %c, whence = %c, start = %d, len = %d\n", type, whence,
	       start, len);
	struct flock fl;
	fl.l_type = char_to_type(type);
	fl.l_whence = char_to_whence(whence);
	fl.l_start = start;
	fl.l_len = len;
	if (fcntl(fd, F_GETLK, &fl) == -1)
		return -1;
	if (fl.l_type == F_UNLCK)
		printf("no lock on this region\n");
	else
		printf("process %d holds the lock\n", (int)fl.l_pid);
	return 0;
}

int
main()
{
	printf("my pid = %d\n", (int) getpid());
	int fd = open("tmp.txt", O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1) {
		printf("error = %s\n", strerror(errno));
		return -1;
	}
	char *buf = NULL;
	size_t size = 0;
	while (getline(&buf, &size, stdin) > 0) {
		char *line = buf;
		line[strlen(line) - 1] = 0;
		int rc;
		printf("line = %s\n", line);
		char *cmd = strsep(&line, " \n");
		printf("args = %s\n", line);
		printf("cmd = %s\n", cmd);
		if (strcmp(cmd, "write") == 0) {
			rc = write_symbols(fd, line);
		} else if (strcmp(cmd, "lock") == 0) {
			rc = do_lock(fd, false, line);
		} else if (strcmp(cmd, "lockb") == 0) {
			rc = do_lock(fd, true, line);
		} else if (strcmp(cmd, "getlock") == 0) {
			rc = get_lock(fd, line);
		} else {
			printf("unknown command\n");
			continue;
		}
		if (rc == -1)
			printf("error = %s\n", strerror(errno));
		else
			printf("ok\n");
	}
	close(fd);
	return 0;
}
