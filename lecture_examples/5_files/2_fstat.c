#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv) {
	struct stat st;
	stat(argv[1], &st);
	printf("inode = %d, protection = %d, links = "\
		"%d, uid = %u, size = %d, blocks = "\
		"%d\n", (int)st.st_ino, (int)st.st_mode,
		(int)st.st_nlink, (unsigned)st.st_uid,
		(int)st.st_size, (int)st.st_blocks);
	if ((st.st_mode & S_IFDIR) == S_IFDIR)
		printf("the file is directory\n");
	if ((st.st_mode & S_IFREG) == S_IFREG)
		printf("the file is regular\n");
	if ((st.st_mode & S_IFLNK) == S_IFLNK)
		printf("the file is symbolic link\n");

	if ((st.st_mode & S_IRUSR) == S_IRUSR)
		printf("can read it\n");
	if ((st.st_mode & S_IWUSR) == S_IWUSR)
		printf("can write it\n");
	if ((st.st_mode & S_IXUSR) == S_IXUSR)
		printf("can execute it\n");

	printf("my uid: %d\n", (int)getuid());
	return 0;
}