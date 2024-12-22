#ifndef __ABOO_LOG_H__
#define __ABOO_LOG_H__

#include <string>
#include <stdint.h>
#include <iostream>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

/**
 * @brief 使用流式方式将日志级别 level 输入到 logger
 *
**/
#define ABOO_LOG_LEVENT(logger, level) \
	if (logger->getLevel() <= level) \
		aboo::LogEventWrap(aboo::LogEvent::ptr(new aboo::LogEvent(logger, level, __FILE__, __LINE__, 0, aboo::getThreadId(), time(0), aboo::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式输入各种级别的日志
 *
**/
#define ABOO_LOG_DEBUG(logger) ABOO_LOG_LEVENT(logger, aboo::LogLevel::DEBUG)
#define ABOO_LOG_INFO(logger) ABOO_LOG_LEVENT(logger, aboo::LogLevel::INFO)
#define ABOO_LOG_WARN(logger) ABOO_LOG_LEVENT(logger, aboo::LogLevel::WARN)
#define ABOO_LOG_ERROR(logger) ABOO_LOG_LEVENT(logger, aboo::LogLevel::ERROR)
#define ABOO_LOG_FATAL(logger) ABOO_LOG_LEVENT(logger, aboo::LogLevel::FATAL)

/**
 * @brief 使用格式化的方式流式传入 logger
 *
**/
#define ABOO_LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if (logger->getLevel() <= level) \
		aboo::LogEventWrap(aboo::LogEvent::ptr(new aboo::LogEvent(logger, level, __FILE__, __LINE__, 0, aboo::getThreadId(), time(0), aboo::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define ABOO_LOG_FMT_DEBUG(logger, fmt, ...) ABOO_LOG_FMT_LEVEL(logger, aboo::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define ABOO_LOG_FMT_INFO(logger, fmt, ...) ABOO_LOG_FMT_LEVEL(logger, aboo::LogLevel::INFO, fmt, __VA_ARGS__)
#define ABOO_LOG_FMT_WARN(logger, fmt, ...) ABOO_LOG_FMT_LEVEL(logger, aboo::LogLevel::WARN, fmt, __VA_ARGS__)
#define ABOO_LOG_FMT_ERROR(logger, fmt, ...) ABOO_LOG_FMT_LEVEL(logger, aboo::LogLevel::ERROR, fmt, __VA_ARGS__)
#define ABOO_LOG_FMT_FATAL(logger, fmt, ...) ABOO_LOG_FMT_LEVEL(logger, aboo::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 *
**/
#define ABOO_LOG_ROOT() aboo::LoggerMgr::getInstance()->getRoot()

/**
 * @brief 获取 name 日志器
 *
**/
#define ABOO_LOG_NAME(name) aboo::LoggerMgr::getInstance()->getLogger(name)

namespace aboo {

class Logger;
class LoggerManager;

// 日志级别
class LogLevel{
public:
	enum Level {
		UNKNOW = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5
	};

	static const char* ToString(LogLevel::Level level);
	static LogLevel::Level FromString(const std::string& str);
};

// 日志事件
class LogEvent {
public:
	typedef std::shared_ptr<LogEvent> ptr;
	LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t m_line, uint32_t elapse, uint32_t thread_id, uint64_t time, const std::string& thread_name);

	/**
	 * @brief 得到文件名
	**/
	const char* getFile() const { return m_file; }

	/**
	 * @brief 得到行号
	**/
	int32_t getLine() const { return m_line; }

	/**
	 * @brief 得到耗时
	**/
	uint32_t getElapse() const { return m_elapse; }

	/**
	 * @brief 得到线程 ID
	**/
	uint32_t getThreadId() const { return m_threadId; }

	/**
	 * @brief 得到返回时间
	**/
	uint64_t getTime() const { return m_time; }

	/**
	 * @brief 得到线程名
	**/
	const std::string& getThreadName() const { return m_threadName; }

	/**
	 * @brief 得到日志内容
	**/
	std::string getContent() const { return m_ss.str(); }

	/**
	 * @brief 得到日志器
	**/
	std::shared_ptr<Logger> getLogger() const { return m_logger; }

	/**
	 * @brief 得到日志级别
	**/
	LogLevel::Level getLevel() const { return m_level; }

	/**
	 * @brief 返回日志内容的字符串流
	**/
	std::stringstream& getSS() { return m_ss; }

	/**
	 * @brief 格式化写入日志内容
	**/
	void format(const char *format, ...);

	/**
	 * @brief 格式化写入日志内容
	**/
	void format(const char *format, va_list al);
private:
	const char* m_file = nullptr;	//文件名
	int32_t m_line = 0;		//行号
	uint32_t m_elapse = 0;		//程序启动开始到现在的毫秒数
	uint32_t m_threadId = 0;	//线程ID
	uint64_t m_time = 0;		//时间戳
	std::string m_threadName;	// 线程名称
	std::stringstream m_ss;		// 日志内容的字符串流
	std::shared_ptr<Logger> m_logger;	// 日志器
	LogLevel::Level m_level;	// 日志级别
};

class LogEventWrap {
public:
	LogEventWrap(LogEvent::ptr e);
	~LogEventWrap();

	/**
	 * @brief 获取日志内容的字符串流
	**/
	std::stringstream& getSS() { return m_event->getSS(); }

	/**
	 * @brief 获取日志事件
	**/
	LogEvent::ptr getEvent() { return m_event; }
private:

	/**
	 * @brief 日志事件
	**/
	LogEvent::ptr m_event;
};

// 日志格式器
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	/**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    **/
	LogFormatter(const std::string& pattern);

	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

	/**
	 * @brief 日志内容项格式化
	**/
	class FormatItem {
	public:
        /**
         * @brief 格式化日志到流
         * @param[in, out] os 日志输出流
         * @param[in] logger 日志器
         * @param[in] level 日志等级
         * @param[in] event 日志事件
         */
		typedef std::shared_ptr<FormatItem> ptr;
		virtual ~FormatItem() { }
		virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
	};

	/**
	 * @brief 初始化，解析日志模板
	**/
	void init();

	/**
	 * @brief 是否有错误
	**/
	bool isError() const { return m_error; }

	/**
	 * @brief 设置日志模板
	 **/
	void setPattern(const std::string& str) {
		m_pattern = str;
	}

	/**
	 * @brief 返回日志模板
	**/
	const std::string getPattern() { return m_pattern; }
private:
	// 日志格式化模板
	std::string m_pattern;
	// 日志格式解析后的格式
	std::vector<FormatItem::ptr> m_items;
	// 是否有错误
	bool m_error = false;
};

// 日志目标输出
class LogAppender {
friend class Logger;
public:
	typedef std::shared_ptr<LogAppender> ptr;
	typedef Spinlock MutexType;
	virtual ~LogAppender() { }

    /**
     * @brief 写入日志
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] event 日志事件
    **/
	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    /**
     * @brief 将日志输出目标的配置转成 YAML String
    **/
	virtual std::string toYamlString() = 0;

    /**
     * @brief 更改日志格式器
    **/
	void setFormatter(LogFormatter::ptr val);

    /**
     * @brief 获取日志格式器
    **/
	LogFormatter::ptr getFormatter();

    /**
     * @brief 获取日志级别
    **/
	LogLevel::Level getLevel() const { return m_level; }

    /**
     * @brief 设置日志级别
    **/
	void setLevel(LogLevel::Level val) { m_level = val; }
protected:
    // 日志级别
	LogLevel::Level m_level;
    // 是否有自己的日志格式器
	bool m_hasFormatter = false;
	// Mutex
	MutexType m_mutex;
	// 日志格式器
	LogFormatter::ptr m_formatter;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
	typedef std::shared_ptr<Logger> ptr;
	typedef Spinlock MutexType;

	Logger(const std::string& name = "root");

	/**
     * @brief 写日志
     * @param[in] level 日志级别
     * @param[in] event 日志事件
    **/
	void log(LogLevel::Level level, LogEvent::ptr event);

    /**
     * @brief 写debug级别日志
     * @param[in] event 日志事件
    **/
	void debug(LogEvent::ptr event);

    /**
     * @brief 写info级别日志
     * @param[in] event 日志事件
    **/
	void info(LogEvent::ptr event);

    /**
     * @brief 写warn级别日志
     * @param[in] event 日志事件
    **/
	void warn(LogEvent::ptr event);

    /**
     * @brief 写error级别日志
     * @param[in] event 日志事件
    **/
	void error(LogEvent::ptr event);

    /**
     * @brief 写fatal级别日志
     * @param[in] event 日志事件
    **/
	void fatal(LogEvent::ptr event);

    /**
     * @brief 添加日志目标
     * @param[in] appender 日志目标
    **/
	void addAppender(LogAppender::ptr appender);

    /**
     * @brief 删除日志目标
     * @param[in] appender 日志目标
    **/
	void delAppender(LogAppender::ptr appender);

    /**
     * @brief 清空日志目标
    **/
	void clearAppenders();

    /**
     * @brief 返回日志名称
    **/
	const std::string& getName() const { return m_name; }

    /**
     * @brief 返回日志格式
    **/
	LogFormatter::ptr getFormatter();

    /**
     * @brief 返回日志级别
    **/
	LogLevel::Level getLevel() const { return m_level; }

    /**
     * @brief 设置日志格式器
    **/
	void setLevel(LogLevel::Level val) { m_level = val; }

    /**
     * @brief 设置日志格式模板
    **/
	void setFormatter(LogFormatter::ptr val);

    /**
     * @brief 设置日志格式模板
    **/
	void setFormatter(std::string val);

    /**
     * @brief 将日志器的配置转成YAML String
    **/
	std::string toYamlString();
private:
	MutexType m_mutex;
	// 日志名称
	std::string m_name;
	// 日志级别
	LogLevel::Level m_level;
	// Appender 集合
	std::list<LogAppender::ptr> m_appenders;
    // 日志目标集合
	LogFormatter::ptr m_formatter;
    // 主日志器
	Logger::ptr m_root;
};

/**
 * @brief 输出到控制台的Appender
**/
class StdoutLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<StdoutLogAppender> ptr;
	void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
	std::string toYamlString() override;
};

/**
 * @brief 输出到文件的Appender
**/
class FileLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<FileLogAppender> ptr;
	void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
	std::string toYamlString() override;
	FileLogAppender(const std::string &filename);
	FileLogAppender() = default;

    /**
     * @brief 重新打开日志文件
     * @return 成功返回true
    **/
	bool reopen();
private:
	// 文件路径
	std::string m_filename;
	// 文件流
	std::ofstream m_filestream;
	// 上次重新打开时间
	uint64_t m_lastTime = 0;
};

/**
 * @brief 日志器管理类
**/
class LoggerManager {
public:
	typedef Spinlock MutexType;
	LoggerManager();

    /**
     * @brief 获取日志器
     * @param[in] name 日志器名称
    **/
	Logger::ptr getLogger(const std::string& name);

    /**
     * @brief 初始化
    **/
	void init();

    /**
     * @brief 返回主日志器
    **/
	Logger::ptr getRoot() const { return m_root; }

    /**
     * @brief 将所有的日志器配置转成YAML String
    **/
	std::string toYamlString();
private:
	MutexType m_mutex;
    // 日志器容器
	std::map<std::string, Logger::ptr> m_loggers;
    // 主日志器
	Logger::ptr m_root;
};

typedef aboo::Singleton<LoggerManager> LoggerMgr;

}
#endif
