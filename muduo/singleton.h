#ifndef __ABOO_SINGLETON_H__
#define __ABOO_SINGLETON_H__

#include <memory>

namespace aboo {

template<class T, class X = void, int N = 0>
class Singleton {
public:
	static T* getInstance() {
		static T v;
		return &v;
	}
};

template<class T, class X = void, int N = 0>
class SingLetonPtr {
public:
	static std::shared_ptr<T> getInstance() {
		static std::shared_ptr<T> v(new T);
		return v;
	}
};

}

#endif
