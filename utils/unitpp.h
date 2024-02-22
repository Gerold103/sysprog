#pragma once

#include <iostream>

class UnitTestCaseGuard
{
public:
	UnitTestCaseGuard(
		std::string_view name)
		: m_name(name)
	{
		std::cout << "\t-------- " << name << " started --------\n";
	}

	~UnitTestCaseGuard()
	{
		std::cout << "\t-------- " << m_name << " done --------\n";
	}

private:
	const std::string m_name;
};

#define unit_test_start() UnitTestCaseGuard test_case_guard(__func__)

#define unit_assert(cond) do {													\
	if (cond) {																	\
		std::cout <<"Test failed, line " << __LINE__ << "\n";					\
		exit(-1);																\
	}																			\
} while (0)

#define unit_msg(...) do {														\
	std::cout << "# " << __VA_ARGS__ << '\n';									\
} while (0)

#define unit_check(cond, msg) do {												\
	if (not (cond)) {															\
		std::cout << "not ok - " << msg << '\n';								\
		unit_assert(false);														\
	} else {																	\
		std::cout << "ok - " << msg << '\n';									\
	}																			\
} while(0)
