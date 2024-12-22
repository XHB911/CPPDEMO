#ifndef __ABOO_NONCOPYABLE_H__
#define __ABOO_NONCOPYABLE_H__

namespace aboo {

class Noncopyable {
public:
	Noncopyable(const Noncopyable&) = delete;
	Noncopyable& operator=(const Noncopyable&) = delete;
protected:
	Noncopyable() = default;
	~Noncopyable() = default;
};

}
#endif
