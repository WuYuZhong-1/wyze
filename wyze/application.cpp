#include "application.h"
#include "log.h"
#include "env.h"
#include "daemon.h"
#include "config.h"
#include "iomanager.h"
#include <unistd.h>

namespace wyze {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

static ConfigVar<std::string>::ptr g_server_work_path = 
    Config::Lookup("server.work_path",
        std::string("/run/wyze")
        , "server work path");

static ConfigVar<std::string>::ptr g_server_pid_file =
    Config::Lookup("server.pid_file"
        ,std::string("wyze.pid")
        ,"server pid file name");

struct HttpServerConf {
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 1000 * 30;
    std::string name;

    bool isValid() const {
        return !address.empty();
    }

    bool operator==(const HttpServerConf& oth) const {
        return address == oth.address
            && keepalive == oth.keepalive
            && timeout == oth.timeout
            && name == oth.name;
    }
};

template<>
class LexicalCast<std::string, HttpServerConf> {
public:
    HttpServerConf operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        HttpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        if(node["address"].IsDefined()) {
            for(size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

template<>
class LexicalCast<HttpServerConf, std::string> {
public:
    std::string operator()(const HttpServerConf& conf) {
        YAML::Node node;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        for(auto& i : conf.address) {
            node["address"].push_back(i);
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

static ConfigVar<std::vector<HttpServerConf>>::ptr g_http_servers_conf
    = Config::Lookup("http_servers", std::vector<HttpServerConf>(), "http server config");

Application::Application()
    : m_argc(1)
    , m_argv(nullptr)
{
}

bool Application::init(int argc, char** argv)
{
    m_argc = argc;
    m_argv = argv;

    Env* singleton = EnvMgr::GetInstance();

    singleton->addHelp("s", "statr with the terminal");
    singleton->addHelp("d", "run as daemon");
    singleton->addHelp("c", "conf path default: ./conf");
    singleton->addHelp("p", "print help");

    if(!singleton->init(argc, argv)) {
        singleton->printHelp();
        return false;
    }

    if(singleton->has("p")) {
        singleton->printHelp();
        return false;
    }

    std::string conf_path = singleton->getAbsolutePath(
                    singleton->get("c", "conf"));
    WYZE_LOG_INFO(g_logger) << "load conf paht:" << conf_path;
    Config::LoadFromConfDir(conf_path);

    //工作目录有两个，一个是作为 daemon 时运行在 /run/wyze
    //一个是作为 terminal 运行在 exe/wyze
    std::string pidfile = g_server_work_path->getValue()
                            + "/" + g_server_pid_file->getValue();
    if(FSUtil::IsRunningPidfile(pidfile)) {
        WYZE_LOG_ERROR(g_logger) << "server is running:" << pidfile;
        return false;
    }

    // pidfile = singleton->getCwd() + "wyze/" + g_server_pid_file->getValue();
    // if(FSUtil::IsRunningPidfile(pidfile)) {
    //     WYZE_LOG_ERROR(g_logger) << "server is running:" << pidfile;
    //     return false;
    // }

    // //
    // if(!EnvMgr::GetInstance()->has("d")) {
    //     g_server_work_path->setVal(singleton->getCwd() + "wyze/");
    // }

    if(!FSUtil::Mkdir(g_server_work_path->getValue())) {
        WYZE_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
            <<"] error=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    return true;
}

bool Application::run()
{
    bool is_daemon = EnvMgr::GetInstance()->has("d");
    return start_daemon(m_argc, m_argv,
                    std::bind(&Application::main, this,
                    std::placeholders::_1,
                    std::placeholders::_2), is_daemon);
}

int Application::main(int argc, char** argv)
{
    {
        std::string pidfile = g_server_work_path->getValue()
                + "/" + g_server_pid_file->getValue();
        std::ofstream ofs(pidfile);
        if(!ofs) {
            WYZE_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
            return false;
        }
        ofs << getpid();
    }

    IOManager iom(2, true, "http");
    iom.schedule(std::bind(&Application::main_fiber, this));
    iom.stop();
    WYZE_LOG_ERROR(g_logger) << "error end";
    return 0;
}

int Application::main_fiber()
{
    auto http_confs = g_http_servers_conf->getValue();
    for(auto& i : http_confs) {
        // WYZE_LOG_INFO(g_logger) << LexicalCast<HttpServerConf, std::string>()(i);

        std::vector<Address::ptr> address;
        for(auto& a : i.address) {
            size_t pos = a.find(":");
            if(pos == std::string::npos) {
                address.push_back(UnixAddress::ptr(new UnixAddress(a)));
                continue;
            }

            int32_t port = atoi(a.substr(pos + 1).c_str());
            auto addr = IPAddress::Create(a.substr(0, pos).c_str(), port);
            if(addr) {
                address.push_back(addr);
                continue;
            }

            std::vector<std::pair<Address::ptr, uint32_t>> result;
            if(!Address::GetInterfaceAddresses(result, a.substr(0, pos))) {
                WYZE_LOG_ERROR(g_logger) << "invalid address: " << a;
                continue; 
            }

            for(auto& x : result) {
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                if(ipaddr && port > 0) {
                    ipaddr->setPort(port);
                }
                address.push_back(ipaddr);
            }
        }

        http::HttpServer::ptr server(new http::HttpServer(i.keepalive));
        std::vector<Address::ptr> fails;
        int count = 0;
        do {
            if(server->bind(address, fails)) {
                break;
            }
            for(auto& j : fails)
                WYZE_LOG_ERROR(g_logger) << "bind address fail" << *j;
            sleep(1);
        }while(++count < 3);

        if(count >= 3)
            exit(0);
        
        if(!i.name.empty())
            server->setName(i.name);
        
        server->setRecvTimeout(i.timeout);
        server->start();
        m_http_servers.push_back(server);
    }

    while(true) {
        WYZE_LOG_INFO(g_logger) << "hello world";
        usleep(1000* 1000 * 60);
    }

    return 0;
}

}