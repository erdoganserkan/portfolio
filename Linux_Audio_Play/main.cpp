#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include "logger.h"
#include "AudioMgr.h"

int main(int argc, char **argv) {
	using namespace std;

	Logger logi("AudioMgrLog", TRACE_LEVEL);
	INFOL((&logi), "stdin/cin is selected:argc(%d)\n", argc);

	AudioMgr aplay(&logi);

	return 0;
}

