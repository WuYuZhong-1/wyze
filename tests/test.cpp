#include <iostream>
#include "../wyze/log.h"
#include "../wyze/config.h"
#include <syscall.h>
#include <thread>


void print()
{
    auto l = wyze::LoggerMgr::GetInstance()->getLogger("XX");
    WYZE_LOG_DEBUG(l) << "-------22----";

    WYZE_LOG_DEBUG(l) << "pthread_self():" << pthread_self();
    WYZE_LOG_DEBUG(l) << "syscall(SYS_gettid):" << syscall(SYS_gettid);
    WYZE_LOG_DEBUG(l) << "std::this_thread::get_id():"<< std::this_thread::get_id();
}

void test_thread()
{
    #if 0
    wyze::Logger::ptr logger(new wyze::Logger);
    logger->addAppender(wyze::LogAppender::ptr(new wyze::StdoutAppender));
    wyze::LogAppender::ptr file_appender(new wyze::FileAppender(std::string("log.txt")));
    file_appender->setLevel(wyze::LogLevel::ERROR);
    // file_appender->setFormatter(wyze::LogFormatter::ptr(new wyze::LogFormatter("%d%T%p%T%m%n")));
    logger->addAppender(file_appender);

    

    WYZE_LOG_DEBUG(logger) << "测试";
    WYZE_LOG_ERROR(logger) << "hello world";

    WYZE_LOG_FMT_DEBUG(logger, "hello %s", "wuyz");
    //2.20

    auto l = wyze::LoggerMgr::GetInstance()->getLogger("XX");
    WYZE_LOG_DEBUG(l) << "------";
    WYZE_LOG_DEBUG(l) << "XX";
    #endif
    wyze::Logger::ptr logger(new wyze::Logger);
    std::cout << "-----------main 1----------" << std::endl;
    WYZE_LOG_DEBUG(logger) << "------11-----";
    
    print();

    std::thread t([](){
        print();
        std::this_thread::sleep_for(std::chrono::seconds(30));
    });

    t.join();
}

static wyze::Logger::ptr g_logger = WYZE_LOG_NAME("system");


void test_log() 
{
    WYZE_LOG_DEBUG(g_logger) << g_logger->toYamlString();
    WYZE_LOG_DEBUG(WYZE_LOG_ROOT()) << "nihoa";
    WYZE_LOG_DEBUG(g_logger) << "hello world";
    YAML::Node root = YAML::LoadFile("/home/wuyz/learn/wyze/bin/config/config.yaml");
    wyze::Config::LoadFromYaml(root);
    g_logger = WYZE_LOG_NAME("XX");
    WYZE_LOG_DEBUG(g_logger) << "HELLO WORLD";
    wyze::Config::Visit([](wyze::ConfigVarBase::ptr var) {
        std::cout << "name=" << var->getName()
                    << " description=" << var->getDescription()
                    << " typename=" << var->getTypeName()
                    << " value=" << var->toString()<< std::endl;
    });
    g_logger = WYZE_LOG_NAME("root");
    WYZE_LOG_DEBUG(g_logger) << g_logger->toYamlString()<< std::endl;
    g_logger = WYZE_LOG_NAME("system");
    WYZE_LOG_DEBUG(g_logger) << g_logger->toYamlString()<< std::endl;

    wyze::PrintLogs()(g_logger);

}

int main(int argc, char **argv)
{
    
    test_log();
    return 0;
}
