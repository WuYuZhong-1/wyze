#ifndef _WYZE_LOG_H_
#define _WYZE_LOG_H_

#include <string>
#include <memory>
#include <stdint.h>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"



#define WYZE_LOG_LEVEL(logger, level)                                                                                   \
            if(logger->getLevel() <= level)                                                                             \
                wyze::LogEventWrap(wyze::LogEvent::ptr(new wyze::LogEvent(logger, level, __FILE__, __LINE__,            \
                                    0, wyze::GetThreadId(), wyze::GetFiberId(), time(0), wyze::Thread::GetName()))).getSS()                    

#define WYZE_LOG_DEBUG(logger) WYZE_LOG_LEVEL(logger, wyze::LogLevel::DEBUG)
#define WYZE_LOG_INFO(logger) WYZE_LOG_LEVEL(logger, wyze::LogLevel::INFO)
#define WYZE_LOG_WARN(logger) WYZE_LOG_LEVEL(logger, wyze::LogLevel::WARING)
#define WYZE_LOG_ERROR(logger) WYZE_LOG_LEVEL(logger, wyze::LogLevel::ERROR)
#define WYZE_LOG_FATAL(logger) WYZE_LOG_LEVEL(logger, wyze::LogLevel::FATAL)

#define WYZE_LOG_FMT_LEVEL(logger, level, fmt, ...)                                                                     \
            if(logger->getLevel() <= level)                                                                             \
                wyze::LogEventWrap(wyze::LogEvent::ptr(new wyze::LogEvent(logger, level, __FILE__, __LINE__,            \
                                    0, wyze::GetThreadId(), wyze::GetFiberId(), time(0),wyze::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)


#define WYZE_LOG_FMT_DEBUG(logger, fmt, ...) WYZE_LOG_FMT_LEVEL(logger, wyze::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define WYZE_LOG_FMT_INFO(logger, fmt, ...) WYZE_LOG_FMT_LEVEL(logger, wyze::LogLevel::INFO, fmt, __VA_ARGS__)
#define WYZE_LOG_FMT_WARN(logger, fmt, ...) WYZE_LOG_FMT_LEVEL(logger, wyze::LogLevel::WARING, fmt, __VA_ARGS__)
#define WYZE_LOG_FMT_ERROR(logger, fmt, ...) WYZE_LOG_FMT_LEVEL(logger, wyze::LogLevel::ERROR, fmt, __VA_ARGS__)
#define WYZE_LOG_FMT_FATAL(logger, fmt, ...) WYZE_LOG_FMT_LEVEL(logger, wyze::LogLevel::FATAL, ftm, __VA__ARGS__)

#define WYZE_LOG_ROOT() wyze::LoggerMgr::GetInstance()->getRoot()
#define WYZE_LOG_NAME(name) wyze::LoggerMgr::GetInstance()->getLogger((name));

namespace wyze {

    class Logger;
    class LoggerManager;

    //????????????
    class LogLevel {
    public:
        enum  Level{
            UNKNOW = 0,
            DEBUG = 1,
            INFO,
            WARING,
            ERROR,
            FATAL
        };
        static const char* ToString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string& str);
    };


    //????????????
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent>  ptr;
        LogEvent(std::shared_ptr<Logger> ptr, LogLevel::Level level, const char* file, int32_t line,
                uint32_t elapse, uint32_t thread_id, uint64_t fiber_id, uint64_t time, const std::string& thread_name);

        const char* getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint64_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        const std::string& getThreadName() const { return m_threadName;}
        std::string getContent() const { return m_ss.str(); }
        std::stringstream& getSS() { return m_ss; }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list val);
    private:
        const char* m_file = nullptr;               //?????????
        int32_t m_line;                             //??????
        uint32_t m_elapse;                          //??????????????????????????????
        uint32_t m_threadId;                        //?????? id
        uint64_t m_fiberId;                         //?????? id
        uint64_t m_time;                            //?????????       
        std::stringstream m_ss;                     //??????
        std::shared_ptr<Logger> m_logger;           //?????????????????????
        LogLevel::Level m_level;                    //??????????????????
        std::string m_threadName;                   //????????????
    };

    class LogEventWrap {
    public:
        LogEventWrap(LogEvent::ptr event);
        ~LogEventWrap();
        std::stringstream& getSS() { return m_event->getSS(); }
        LogEvent::ptr& getEvent() { return m_event; }
    private:
        LogEvent::ptr m_event;
    };

    //???????????????
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string& pattern);
        std::string format(LogEvent::ptr event);
        void init();
        const std::string getPattern() const { return m_pattern;}
        bool isError() const { return m_error; }
    public:
        class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem() {}
            virtual void format(std::ostream& os,LogEvent::ptr event) = 0;
        };


    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_item;
        bool m_error = false;
    };

    //???????????????
    class LogAppender {
        friend class Logger;
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        using MutexType = SpinLock;
        virtual ~LogAppender(){};

        virtual void log(LogEvent::ptr event) = 0;
        virtual std::string toYamlString() = 0;

        void setFormatter(LogFormatter::ptr val) { 
            MutexType::Lock lock(m_mutex);
            m_formatter= val; 
            if(m_formatter) {
                m_hasFormatter = true;
            } else {
                m_hasFormatter = false;
            }
        }
        LogFormatter::ptr getFormatter() const { return m_formatter; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        LogLevel::Level getLevel() const { return m_level; }

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        LogFormatter::ptr m_formatter;
        bool m_hasFormatter = false;
        MutexType m_mutex;
    };

    //?????????
    class Logger: public std::enable_shared_from_this<Logger> {
        friend class LoggerManager;
    public:
        typedef std::shared_ptr<Logger> ptr;
        using MutexType = SpinLock; 

        Logger(const std::string& name ="root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();
        LogLevel::Level getLevel() const { return m_level;}
        void setLevel(LogLevel::Level level) { m_level = level; };
        const std::string& getName() const { return m_name; }

        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string& val);
        LogFormatter::ptr getFormatter();

        std::string toYamlString();
    private:
        std::string m_name;                                 //????????????
        LogLevel::Level m_level;                            //????????????
        MutexType m_mutex;
        std::list<LogAppender::ptr> m_appenders;            //appender??????
        LogFormatter::ptr m_format;                         //????????????appender??????format???????????????format
        Logger::ptr m_root;                                 //??????logger??????????????? root log
    };


    //????????????????????? Appender
    class StdoutAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutAppender> ptr;
        virtual void log(LogEvent::ptr event) override;
        std::string toYamlString() override;
    private:
        
    };

    //?????????????????? Appender
    class FileAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileAppender> ptr;
        FileAppender(const std::string &filename);
        //??????????????????????????? true
        bool reopen();
        virtual void log(LogEvent::ptr event) override;
        std::string toYamlString() override;
    private:
        std::string m_filename;
        std::ofstream m_filestream;
        uint64_t m_lastTime = 0;
    };

    class LoggerManager {
    public:
        using MutexType = SpinLock;
        LoggerManager();
        Logger::ptr getLogger(const std::string name);
        Logger::ptr getRoot() const { return m_root; }
        std::string toYamlString();
    private:
        MutexType m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    typedef wyze::Single<LoggerManager> LoggerMgr;


    class PrintLogs{
    public:
        void operator()(Logger::ptr log) ;
    };

}

#endif // _WYZE_LOG_H_