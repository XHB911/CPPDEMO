#ifndef __UTIL_H__
#define __UTIL_H__

#define LOG(str) \
	std::cout << __FILE__ << " : " << __LINE__ << " : " << \
	__TIMESTAMP__ << ": " << str << endl;

#endif
