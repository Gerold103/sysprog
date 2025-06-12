#include "userfs.h"

#include "rlist.h"

#include <list>
#include <stddef.h>
#include <vector>

enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 100,
};

// Global error code. Set from any function on any error.
static ufs_error_code glob_ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	// Block memory.
	std::vector<uint8_t> data;

	// PUT HERE OTHER MEMBERS
};

struct file {
	// Sequential blocks of the file.
	std::vector<block> blocks;
	// How many file descriptors are opened on the file.
	int refs;
	// File name.
	std::string name;

	// PUT HERE OTHER MEMBERS
};

// List of all files.
static std::list<file *> file_list;

struct filedesc {
	file *file = nullptr;

	// PUT HERE OTHER MEMBERS
};

// An array of file descriptors. When a file descriptor is created, its pointer is added
// here. When a file descriptor is closed, its place in this array is set to nullptr and
// can be taken by next ufs_open() call.
static std::vector<filedesc *> file_descriptors;

ufs_error_code
ufs_errno()
{
	return glob_ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{
	// IMPLEMENT THIS FUNCTION
	(void)filename;
	(void)flags;
	(void)file_list;
	(void)file_descriptors;
	glob_ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

ssize_t
ufs_write(int fd, const char *buf, size_t size)
{
	// IMPLEMENT THIS FUNCTION
	(void)fd;
	(void)buf;
	(void)size;
	glob_ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
	// IMPLEMENT THIS FUNCTION
	(void)fd;
	(void)buf;
	(void)size;
	glob_ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

int
ufs_close(int fd)
{
	// IMPLEMENT THIS FUNCTION
	(void)fd;
	glob_ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

int
ufs_delete(const char *filename)
{
	// IMPLEMENT THIS FUNCTION
	(void)filename;
	glob_ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

#if NEED_RESIZE

int
ufs_resize(int fd, size_t new_size)
{
	// IMPLEMENT THIS FUNCTION
	(void)fd;
	(void)new_size;
	glob_ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

#endif

void
ufs_destroy(void)
{
}
