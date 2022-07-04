#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include "logger.h"

int main(int argc, char **argv) {
	using namespace std;

	Logger logi("TestCppLogFile", INFO_LEVEL);
	ERRL(logi, "First log entry(%d)\n", 5);

	return EXIT_SUCCESS;
}

