#ifndef __ABOO_UTIL_H__
#define __ABOO_UTIL_H__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <time.h>

namespace aboo {

pid_t getThreadId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

// 时间ms/us
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();

std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

class FSUtil {
public:
	static void ListAllFile(std::vector<std::string>& files
							, const std::string& path
							, const std::string& subfix);
	static bool Mkdir(const std::string& dirname);
	static bool IsRunningPidfile(const std::string& pidfile);
};

}

#endif
