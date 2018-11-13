#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

void
print_all_sems(int semid)
{
	unsigned short values[3];
	if (semctl(semid, -1, GETALL, values) == -1) {
		printf("error = %s\n", strerror(errno));
	} else {
		printf("Values are %d %d %d\n", values[0], values[1],
		       values[2]);
	}
}

void
init_sems(int semid)
{
	unsigned short values[3] = {1, 2, 3};
	semctl(semid, -1, SETALL, values);
	printf("Sem array is initialized\n");
	print_all_sems(semid);
}

int
main()
{
	key_t key = ftok("./a.out", 0);
	int semid = semget(key, 3, IPC_CREAT | IPC_EXCL | S_IRWXU | S_IRWXO);

	if (semid == -1) {
		if (errno != EEXIST) {
			printf("error = %s\n", strerror(errno));
			return -1;
		}
		printf("Sem array already exists\n");
		semid = semget(key, 0, 0);
		if (semid == -1) {
			printf("error = %s\n", strerror(errno));
			return -1;
		}
		printf("Connected to %d\n", semid);
	} else {
		printf("Created sems with id %d\n", semid);
		init_sems(semid);
	}

	struct sembuf ops[100], *op;
	int ops_count = 0, sem_num;
	bool is_inside_txn = false;
	char cmd, *line = NULL;
	size_t line_size = 0;
	while (1) {
		getline(&line, &line_size, stdin);
		if (line == NULL)
			continue;
		/* Trim '\n'. */
		line[strlen(line) - 1] = 0;
		if (strcmp(line, "begin") == 0) {
			is_inside_txn = true;
		} else if (strcmp(line, "commit") == 0) {
			if (semop(semid, ops, ops_count) == -1) {
				printf("error = %s\n", strerror(errno));
			} else {
				is_inside_txn = false;
				ops_count = 0;
				print_all_sems(semid);
			}
		} else if (strcmp(line, "rollback") == 0) {
			is_inside_txn = false;
			ops_count = 0;
		} else if (strcmp(line, "delete") == 0) {
			if (semctl(semid, -1, IPC_RMID) == -1)
				printf("error = %s\n", strerror(errno));
			return 0;
		} else if (strcmp(line, "quit") == 0) {
			return 0;
		} else if (strcmp(line, "show") == 0) {
			print_all_sems(semid);
		} else {
			sscanf(line, "%d %c", &sem_num, &cmd);
			op = &ops[ops_count];
			if (cmd == '-') {
				op->sem_op = -1;
			} else if (cmd == '+') {
				op->sem_op = 1;
			} else {
				printf("Unknown operation\n");
				continue;
			}
			op->sem_num = sem_num - 1;
			op->sem_flg = SEM_UNDO;
			if (! is_inside_txn) {
				if (semop(semid, op, 1) == -1)
					printf("error = %s\n", strerror(errno));
				else
					print_all_sems(semid);
			} else {
				ops_count++;
			}
		}
	}
	return 0;
}
