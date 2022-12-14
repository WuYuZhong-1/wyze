#include "log.h"
#include <iostream>
#include <map>
#include <functional>
#include <cstring>
#include <ctime>
#include "config.h"

namespace wyze {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

LogEvent::LogEvent(Logger::ptr logger, LogLevel::Level level, const char* file, int32_t line,
                uint32_t elapse, uint32_t thread_id, uint64_t fiber_id, uint64_t time, const std::string& thread_name)
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(thread_id)
    ,m_fiberId(fiber_id)
    ,m_time(time)
    ,m_logger(logger)
    ,m_level(level)
    ,m_threadName(thread_name)
    
{
}

void LogEvent::format(const char *fmt, ...)
{
    va_list vaList;
    va_start(vaList, fmt);
    format(fmt, vaList);
    va_end(vaList);
}

void LogEvent::format(const char *fmt, va_list val)
{
    char *buf = nullptr;
    int len = vasprintf(&buf, fmt, val);
    if( len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr event)
    :m_event(event)
{
}
LogEventWrap::~LogEventWrap()
{
    // std::cout << "-------------" << std::endl;
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

const char* LogLevel::ToString(LogLevel::Level level)
{
    switch(level) {
#define XX(name)            \
    case LogLevel::name:    \
        return #name;       \
        break;                  

    XX(DEBUG);
    XX(INFO);
    XX(WARING);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str)
{
#define XX(l, v)    \
    if(str == #v){   \
        return LogLevel::Level::l;   \
    }

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARING, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARING, WARING);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::Level::UNKNOW;
#undef XX
}

Logger::Logger(const std::string& name)
    :m_name(name),
    m_level(LogLevel::Level::DEBUG)
{
    m_format.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // if(name == "root") {
    //     addAppender(LogAppender::ptr(new StdoutAppender));
    // }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level) {

        if(!m_appenders.empty()) {
            //auto self = shared_from_this();
            MutexType::Lock lock(m_mutex);
            for(auto& ite : m_appenders) {
                //ite->log(Logger::ptr(this) ,level, event);
                ite->log(event);
            }
        }
        else if(m_root) {
            m_root->log(level,event);
        }
    }
}

void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARING, event);
}

void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}

void Logger::addAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    //??????appender ?????? format ?????????logger ???format
    if(!appender->getFormatter()) {
        appender->setFormatter(m_format);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    for( auto ite = m_appenders.begin();
            ite != m_appenders.end(); ++ite) {

        if( *ite == appender) {
            m_appenders.erase(ite);
            break;
        }
            
    }
}

void Logger::clearAppenders()
{
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}


void Logger::setFormatter(LogFormatter::ptr val)
{
    MutexType::Lock lock(m_mutex);
    m_format = val;

    for(auto& i : m_appenders) {
        MutexType::Lock ll(i->m_mutex);
        if(!i->m_hasFormatter) {
            i->m_formatter = m_format;
        }
    }
}

void Logger::setFormatter(const std::string& val)
{
    LogFormatter::ptr new_val(new LogFormatter(val));
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name = " << m_name
                    << "value = " << val << "invaild formatter"
                    << std::endl;

        return;
    }
    // m_format = new_val;
    setFormatter(new_val);

}

std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_format) {
        node["formatter"] = m_format->getPattern();
    }

    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::ptr Logger::getFormatter() 
{
    MutexType::Lock lock(m_mutex);
    return m_format;
}


void StdoutAppender::log(LogEvent::ptr event)
{
    if(event->getLevel() >= m_level) {
        MutexType::Lock lock(m_mutex);
        std::cout << m_formatter->format(event);
    }
}

std::string StdoutAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

FileAppender::FileAppender(const std::string &filename)
    :m_filename(filename)
{
    if(!reopen()) {
        std::cout << "not open file: " << filename << std::endl;
    }
}

bool FileAppender::reopen()
{
    MutexType::Lock lock(m_mutex);
    if(m_filestream.is_open()) {
        m_filestream.close();
    }
    
    return FSUtil::OpenForWrite(m_filestream, m_filename,std::ios::app);
}

std::string FileAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void FileAppender::log(LogEvent::ptr event)
{
    if(event->getLevel() >= m_level) {
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3)) {
            if(!reopen()) {
                std::cout << "not open file: " << m_filename << std::endl;
            }
            m_lastTime = now;
        }
        MutexType::Lock lock(m_mutex);
        m_filestream << m_formatter->format(event);
    }
}

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern)
{
    init();
}

std::string LogFormatter::format(LogEvent::ptr event)
{
    std::stringstream ss;
    for( auto ite : m_item) {
        ite->format(ss, event);
    }
    return ss.str();
}

// %xxx %xxx{xxx} %%
void LogFormatter::init()
{
    //"%d [%p] %f %l %m %n"
    // sting format type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i) {

        //???????????? ???%' ???????????????????????????
        if(m_pattern[i] != '%') {   
            nstr.append(1, m_pattern[i]);
            continue;
        }

        //????????????????????? ??????????????? i ????????? '%' ??????

        //?????????????????????????????? ???%?????? ??????????????????????????????????????? ????????? '%'
        if((i + 1) < m_pattern.size()) {
            if( m_pattern[i + 1] == '%') {
                nstr.append(1, m_pattern[i]);
                i += 1; //?????????????????????
                continue;
            }
        }

        size_t n = i + 1;       //???%??? ????????????????????????
        int fmt_status = 0;     
        size_t fmt_start = 0;
        std::string str;
        std::string fmt;

        while(n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}') ) {
                str = m_pattern.substr(i+1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i+1, n-i-1);
                    fmt_status = 1;
                    fmt_start = n;
                    ++n;
                    continue;
                }
            }
            else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_start +1, n - fmt_start -1);
                    fmt_status =0;
                    ++n;
                    break;
                }
            }
            ++n;
            if( n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i+1);
                }
            }
        } 

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str,fmt, 1));
            i = n - 1;
        }
        else if(fmt_status == 1) {
            std::cout << "pattern parse error" << m_pattern << "-" << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern _error>>", fmt, 0));
        }
        // else if(fmt_status == 2) {
        //     if(!nstr.empty()) {
        //         vec.push_back(std::make_tuple(nstr,std::string(), 0));
        //         nstr.clear();
        //     }
        //     vec.push_back(std::make_tuple(str, fmt, 1));
        //     i = n - 1 ;
        // }
        
    }

    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
    }

    // %m -- ?????????
    // %p -- level
    // %r -- ??????????????????
    // %c -- ????????????
    // %t -- ?????????
    // %n -- ????????????
    // %d -- ??????
    // %f -- ?????????
    // %l -- ??????

class MessageFormatItem: public FormatItem {
public:
    // typedef std::shared_ptr<MessageFormatItem> ptr;
    MessageFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem: public FormatItem {
public:
    // typedef std::shared_ptr<LevelFormatItem> ptr;
    LevelFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << LogLevel::ToString(event->getLevel());
    }

};

class ElapseFormatItem: public FormatItem {
public:
    ElapseFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem: public FormatItem {
public:
    NameFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem: public FormatItem {
public:
    ThreadIdFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem: public FormatItem {
public:
    FiberIdFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem: public FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") 
        : m_format(format) { 
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        
        char str[100] = {0};
        const time_t time = event->getTime();
#if 1
        struct tm tm;
        localtime_r(&time, &tm);
        strftime(str, sizeof(str), m_format.c_str(), &tm);
#else
        ctime_r(&time, str);
        str[strlen(str) - 2] = '\0';
#endif
        os << str;
    }
private:
    std::string m_format;
};

class FilenameFormatItem: public FormatItem {
public:
    FilenameFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem: public FormatItem {
public:
    LineFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem: public FormatItem {
public:
    NewLineFormatItem(const std::string &str = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem: public FormatItem {
public:
    StringFormatItem(const std::string& str) 
        : m_string(str) { }
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << m_string;
    } 
private:
    std::string m_string;
};

class TabFormatItem: public FormatItem {
public:
    TabFormatItem(const std::string& str  = "") {}
    virtual void format(std::ostream& os, LogEvent::ptr event) override {
        os << "\t";
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void  format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_item = {
#define XX(str, C)  \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); } }

        XX(m, MessageFormatItem),           //m:??????
        XX(p, LevelFormatItem),             //p:????????????
        XX(r, ElapseFormatItem),            //r:???????????????
        XX(c, NameFormatItem),              //c:????????????
        XX(t, ThreadIdFormatItem),          //t:??????id
        XX(n, NewLineFormatItem),           //n:??????
        XX(d, DateTimeFormatItem),          //d:??????
        XX(f, FilenameFormatItem),          //f:?????????
        XX(l, LineFormatItem),              //l:??????
        XX(T, TabFormatItem),               //T???tab
        XX(F, FiberIdFormatItem),           //F:??????id
        XX(N, ThreadNameFormatItem)        //N:????????????

#undef XX
    };

    for( auto i: vec) {
        if(std::get<2>(i) == 0) {
            m_item.push_back( FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else {
            auto it = s_format_item.find(std::get<0>(i));
            if(it == s_format_item.end() ) {
                m_error = true;
                m_item.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %"+ std::get<0>(i) + ">>")));
            }
            else {
                m_item.push_back(it->second(std::get<1>(i)));
            }
        }
        //std::cout << "(" << std::get<0>(i)<<")" <<  "-" << "(" << std::get<1>(i) <<")" <<"-" << "(" << std::get<2>(i) << ")" << std::endl;
    } 
}

LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    //?????????root logger??????????????? appender
    m_root->addAppender(LogAppender::ptr(new StdoutAppender));

    m_loggers[m_root->m_name] = m_root;

}

struct LogAppenderDefine {
    int type = 0;   // 1 File, 2  Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
                && level == oth.level
                && formatter == oth.formatter
                && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
                && level == oth.level
                && formatter == oth.formatter
                && appenders == oth.appenders;  //TODO:??????????????????==
    }

    //????????????????????????????????????
    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }
};

template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node root = YAML::Load(v);
        LogDefine ret;
        ret.name = root["name"].as<std::string>();
        ret.level = LogLevel::FromString(root["level"].IsDefined() ? root["level"].as<std::string>() : "");
        if(root["formatter"].IsDefined())
            ret.formatter = root["formatter"].as<std::string>();
        
        if(root["appenders"].IsDefined()) {

            for(size_t i = 0; i < root["appenders"].size(); ++i) {
                auto node = root["appenders"][i];
                LogAppenderDefine a;
                if(!node["type"].IsDefined()) {
                   WYZE_LOG_ERROR(g_logger) << "log config error: appender type is null, " << node;
                        continue; 
                }
                
                std::string type = node["type"].as<std::string>();
                if(type == "FileLogAppender") {
                    a.type = 1;
                    if(!node["file"].IsDefined()) {
                        WYZE_LOG_ERROR(g_logger) << "log config error: fileappender file is null, " << node;
                        continue;
                    }
                    a.file = node["file"].as<std::string>();
                    
                } else if(type == "StdoutLogAppender") {
                    a.type = 2;
                } else {
                    WYZE_LOG_ERROR(g_logger) << "log config error: appender type is invalid, " << node;
                    continue;
                }

                if(node["formatter"].IsDefined()) {
                    a.formatter = node["formatter"].as<std::string>();
                }

                ret.appenders.push_back(a);
            }
            
        }
        return ret;
    }
};

template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        if(i.level != LogLevel::UNKNOW) {
            n["level"] = LogLevel::ToString(i.level);
        }
        if(!i.formatter.empty()) {
            n["formatter"] = i.formatter;
        }

        for(auto& a : i.appenders) {
            YAML::Node na;
            if(a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if(a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
            if(a.level != LogLevel::UNKNOW) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if(!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
            
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

#if 0
template<>
class LexicalCast<std::string, std::set<LogDefine> > {
public:
    std::set<LogDefine> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> vec;
        //node["name"].IsDefined()
        for(size_t i = 0; i < node.size(); ++i) {
            auto n = node[i];
            if(!n["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                continue;
            }

            LogDefine ld;
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if(n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            if(n["appenders"].IsDefined()) {
                //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
                for(size_t x = 0; x < n["appenders"].size(); ++x) {
                    auto a = n["appenders"][x];
                    if(!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, " << a
                                  << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if(type == "FileLogAppender") {
                        lad.type = 1;
                        if(!a["file"].IsDefined()) {
                            std::cout << "log config error: fileappender file is null, " << a
                                  << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if(a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else if(type == "StdoutLogAppender") {
                        lad.type = 2;
                    } else {
                        std::cout << "log config error: appender type is invalid, " << a
                                  << std::endl;
                        continue;
                    }

                    ld.appenders.push_back(lad);
                }
            }
            //std::cout << "---" << ld.name << " - "
            //          << ld.appenders.size() << std::endl;
            vec.insert(ld);
        }
        return vec;
    }
};

template<>
class LexicalCast<std::set<LogDefine>, std::string> {
public:
    std::string operator()(const std::set<LogDefine>& v) {
        YAML::Node node;
        for(auto& i : v) {
            YAML::Node n;
            n["name"] = i.name;
            if(i.level != LogLevel::UNKNOW) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if(i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for(auto& a : i.appenders) {
                YAML::Node na;
                if(a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if(a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if(a.level != LogLevel::UNKNOW) {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if(!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

#endif 

wyze::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
    wyze::Config::Lookup("logs", std::set<LogDefine>(), "logs config");


void PrintLogs::operator()(Logger::ptr log)
{
    auto it = Config::Lookup<std::set<LogDefine>>("logs");
    WYZE_LOG_INFO(g_logger) << it->toString(); 
}

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                    const std::set<LogDefine>& new_value){
            WYZE_LOG_INFO(g_logger) << "on_logger_conf_changed";
            for(auto& i : new_value) {
                // if(i.name == "root")
                //     continue;               //TODO:??????root?????????

                auto it = old_value.find(i);
                wyze::Logger::ptr logger;
                if(it == old_value.end()) {
                    //??????logger
                    logger = WYZE_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //?????????logger
                        logger = WYZE_LOG_NAME(i.name);
                    }
                }
                logger->setLevel(i.level);
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();

                for(auto& a : i.appenders) {
                    wyze::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new StdoutAppender);
                    }
                    ap->setLevel(a.level);
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }

                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //??????logger
                    auto logger = WYZE_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;

Logger::ptr LoggerManager::getLogger(const std::string name)
{
    // std::cout << "LoggerManager::getLogger run";
    MutexType::Lock lock(m_mutex);
    auto ite = m_loggers.find(name);
    if(ite != m_loggers.end()) {
        return ite->second;
    }
    
    Logger::ptr log(new Logger(name));
    log->m_root = m_root;       //????????????????????????
    m_loggers[name] = log;

    return log;
}


std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


};