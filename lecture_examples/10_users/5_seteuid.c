#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

uid_t uid, euid, suid;

void
show_uids()
{
	getresuid(&uid, &euid, &suid);
	printf("uid = %d, euid = %d, suid = %d\n", (int) uid, (int) euid,
	       (int) suid);
}

int
main(int argc, const char **argv)
{
	uid_t new_uid1 = atoi(argv[1]);
	uid_t new_uid2 = atoi(argv[2]);
	uid_t new_uid3 = atoi(argv[3]);
	show_uids();
	printf("Set EUID to %d\n", (int) new_uid1);
	if (seteuid(new_uid1) == -1)
		printf("error = %s\n", strerror(errno));

	show_uids();
	printf("Set EUID to %d\n", (int) new_uid2);
	if (seteuid(new_uid2) == -1)
		printf("error = %s\n", strerror(errno));

	show_uids();
	printf("Set UID to %d\n", (int) new_uid3);
	if (setuid(new_uid3) == -1)
		printf("error = %s\n", strerror(errno));

	show_uids();
	return 0;
}
