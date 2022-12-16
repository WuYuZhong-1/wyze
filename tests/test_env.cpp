#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

int main(int argc, char** argv)
{
    wyze::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    wyze::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    wyze::EnvMgr::GetInstance()->addHelp("p", "print help");

    if(!wyze::EnvMgr::GetInstance()->init(argc, argv)) {
        wyze::EnvMgr::GetInstance()->printHelp();
        return -1;
    }

    if(wyze::EnvMgr::GetInstance()->has("p")) {
        wyze::EnvMgr::GetInstance()->printHelp();
    }

    std::cout << "exe=" << wyze::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << wyze::EnvMgr::GetInstance()->getCwd() << std::endl;
    std::cout << "program=" << wyze::EnvMgr::GetInstance()->getProgram() << std::endl;

    return 0;
}