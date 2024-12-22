#ifndef __ABOO_THREAD_H__
#define __ABOO_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <semaphore.h>
#include <atomic>

#include "noncopyable.h"

namespace aboo {

/**
 * @brief 信号量
 */
class Semaphore : Noncopyable {
public:

    /**
     * @brief 构造函数
     * @param[in] count 信号量值的大小
     */
	Semaphore(uint32_t count = 0);
	~Semaphore();

    /**
     * @brief 获取信号量
     */
	void wait();

    /**
     * @brief 释放信号量
     */
	void notify();
private:
	sem_t m_semaphore;
};

/**
 * @brief 局部锁的模板实现
 */
template<class T>
struct ScopedLockImpl {
public:

    /**
     * @brief 构造函数
     * @param[in] mutex Mutex
     */
	ScopedLockImpl(T& mutex) : m_mutex(mutex) {
		m_mutex.lock();
		m_locked = true;
	}


    /**
     * @brief 析构函数,自动释放锁
     */
	~ScopedLockImpl() {
		unlock();
	}


    /**
     * @brief 加锁
     */
	void lock() {
		if (!m_locked) {
			m_mutex.lock();
			m_locked = true;
		}
	}

    /**
     * @brief 解锁
     */
	void unlock() {
		if (m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}
private:
	T& m_mutex;
	bool m_locked;
};

/**
 * @brief 局部读锁模板实现
 */
template<class T>
struct ReadScopedLockImpl {
public:


    /**
     * @brief 构造函数
     * @param[in] mutex 读写锁
     */
	ReadScopedLockImpl(T& mutex) : m_mutex(mutex) {
		m_mutex.rdlock();
		m_locked = true;
	}

    /**
     * @brief 析构函数,自动释放锁
     */
	~ReadScopedLockImpl() {
		unlock();
	}

    /**
     * @brief 上读锁
     */
	void lock() {
		if (!m_locked) {
			m_mutex.rdlock();
			m_locked = true;
		}
	}

    /**
     * @brief 释放锁
     */
	void unlock() {
		if (m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}
private:
	T& m_mutex;
	bool m_locked;
};

/**
 * @brief 局部写锁模板实现
 */
template<class T>
struct WriteScopedLockImpl {
public:
	/**
     * @brief 构造函数
     * @param[in] mutex 读写锁
     */
	WriteScopedLockImpl(T& mutex) : m_mutex(mutex) {
		m_mutex.wrlock();
		m_locked = true;
	}

    /**
     * @brief 析构函数
     */
	~WriteScopedLockImpl() {
		unlock();
	}

    /**
     * @brief 上写锁
     */
	void lock() {
		if (!m_locked) {
			m_mutex.wrlock();
			m_locked = true;
		}
	}

    /**
     * @brief 解锁
     */
	void unlock() {
		if (m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}
private:
	T& m_mutex;
	bool m_locked;
};

/**
 * @brief 互斥量
 */
class Mutex : Noncopyable {
public:
    // 局部锁
	typedef ScopedLockImpl<Mutex> Lock;

    /**
     * @brief 构造函数
     */
	Mutex() {
		pthread_mutex_init(&m_mutex, nullptr);
	}

    /**
     * @brief 析构函数
     */
	~Mutex() {
		pthread_mutex_destroy(&m_mutex);
	}

    /**
     * @brief 加锁
     */
	void lock() {
		pthread_mutex_lock(&m_mutex);
	}
    /**
     * @brief 解锁
     */
	void unlock() {
		pthread_mutex_unlock(&m_mutex);
	}
private:
	pthread_mutex_t m_mutex;
};

/**
 * @brief 空锁(用于调试)
 */
class NullMutex : Noncopyable {
public:
	typedef ScopedLockImpl<NullMutex> Lock;
	NullMutex() {}
	~NullMutex() {}
	void lock() {}
	void unlock() {}
};

/**
 * @brief 读写互斥量
 */
class RWMutex : Noncopyable {
public:
	typedef ReadScopedLockImpl<RWMutex> ReadLock;
	typedef WriteScopedLockImpl<RWMutex> WriteLock;

    /**
     * @brief 构造函数
     */
	RWMutex() {
		pthread_rwlock_init(&m_lock, nullptr);
	}

    /**
     * @brief 析构函数
     */
	~RWMutex() {
		pthread_rwlock_destroy(&m_lock);
	}

    /**
     * @brief 上读锁
     */
	void rdlock() {
		pthread_rwlock_rdlock(&m_lock);
	}

    /**
     * @brief 上写锁
     */
	void wrlock() {
		pthread_rwlock_wrlock(&m_lock);
	}

    /**
     * @brief 解锁
     */
	void unlock() {
		pthread_rwlock_unlock(&m_lock);
	}
private:
	pthread_rwlock_t m_lock;
};

/**
 * @brief 空读写锁(用于调试)
 */
class NullRWMutex : Noncopyable {
public:
	typedef ReadScopedLockImpl<NullMutex> ReadLock;
	typedef WriteScopedLockImpl<NullMutex> WriteLock;

	NullRWMutex() {}
	~NullRWMutex() {}
	void rdlcok() {}
	void wrlock() {}
	void unlock() {}
};

/**
 * @brief 自旋锁
 */
class Spinlock : Noncopyable {
public:
	typedef ScopedLockImpl<Spinlock> Lock;

    /**
     * @brief 构造函数
     */
	Spinlock() {
		pthread_spin_init(&m_mutex, 0);
	}

    /**
     * @brief 析构函数
     */
	~Spinlock() {
		pthread_spin_destroy(&m_mutex);
	}

    /**
     * @brief 上锁
     */
	void lock() {
		pthread_spin_lock(&m_mutex);
	}

    /**
     * @brief 解锁
     */
	void unlock() {
		pthread_spin_unlock(&m_mutex);
	}
private:
	pthread_spinlock_t m_mutex;
};
/*
class CASLock : Noncopyable {
public:
	typedef ScopedLockImpl<CASLock> Lock;
	CASLock() {
		m_mutex.clear();
	}
	~CASLock() {

	}
	void lock() {
		while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
	void unlock() {
		std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
	}
private:
	volatile std::atomic_flag m_mutex;
};
*/

/**
 * @brief 线程类
 */
class Thread {
public:
	typedef std::shared_ptr<Thread> ptr;

    /**
     * @brief 构造函数
     * @param[in] cb 线程执行函数
     * @param[in] name 线程名称
     */
	Thread(std::function<void()> cb, const std::string& name);

    /**
     * @brief 析构函数
     */
	~Thread();

    /**
     * @brief 线程ID
     */
	pid_t getId() const { return m_id; }

    /**
     * @brief 线程名称
     */
	const std::string& getName() const { return m_name; }

    /**
     * @brief 等待线程执行完成
     */
	void join();

    /**
     * @brief 获取当前的线程指针
     */
	static Thread* GetThis();

    /**
     * @brief 获取当前的线程名称
     */
	static const std::string& GetName();

    /**
     * @brief 设置当前线程名称
     * @param[in] name 线程名称
     */
	static void SetName(const std::string& name);
private:

	Thread(const Thread&) = delete;

	Thread(const Thread&&) = delete;

	Thread& operator= (const Thread&) = delete;

    /**
     * @brief 线程执行函数
     */
	static void* run(void *arg);

    // 线程id
	pid_t m_id = -1;
	// 线程结构
	pthread_t m_thread = 0;
	// 线程执行函数
	std::function<void()> m_cb;
	// 线程名称
	std::string m_name;
	// 信号量
	Semaphore m_semaphore;
};

}

#endif
