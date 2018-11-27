#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>

int
main()
{
	uid_t uid, euid, suid;
	getresuid(&uid, &euid, &suid);
	printf("uid = %d, euid = %d, suid = %d\n", (int) uid, (int) euid,
	       (int) suid);
	return 0;
}
