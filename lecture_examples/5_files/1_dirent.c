#include <stdio.h>
#include <dirent.h>

int main()
{
	DIR *dir = opendir(".");
	struct dirent *dirent = readdir(dir);
	while (dirent != NULL) {
		printf("name = %s, inode number = %d, type = %d\n",
		       dirent->d_name, (int) dirent->d_ino,
		       (int) dirent->d_type);
		dirent = readdir(dir);
	}
	closedir(dir);
	return 0;
}
