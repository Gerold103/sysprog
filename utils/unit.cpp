#include "unit.h"

#include <string.h>

bool
doCmdMaxPoints(int argc, char **argv)
{
	for (int i = 0; i < argc; ++i)
	{
		if (strcmp(argv[i], "--max_points") == 0)
		{
			return true;
		}
	}
	return false;
}
