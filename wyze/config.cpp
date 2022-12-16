#include "config.h"
#include "util.h"
#include "env.h"
#include <list>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace wyze {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string& name) 
{
    RWMutexType::ReadLock lock(GetMutex());
    auto ite = GetDatas().find(name);
    return ite != GetDatas().end() ? ite->second : nullptr;
}

static void ListAllMember(const std::string& prefix, const YAML::Node& node,
                            std::list<std::pair<std::string, const YAML::Node>>& output)
{
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
        WYZE_LOG_ERROR(g_logger) << "Config invalid name : " << prefix << " : " << node;
        return;
    }

    //TODO:这里存在问题，需要是底层的scalar 才保存
    output.push_back(std::make_pair(prefix, node));
    if(node.IsMap()) {
        for(auto ite = node.begin(); ite != node.end(); ++ite) {
            ListAllMember(prefix.empty() ? ite->first.Scalar() :
                prefix + "." + ite->first.Scalar(), ite->second, output);
        }
    } 
    // else if( node.IsSequence()) {
    //     for(int i = 0; i < node.size(); ++i) {
    //         ListAllMember(prefix, node[i], output);
    //     }
    // }
}


void Config::LoadFromYaml(const YAML::Node& root) 
{
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes); 

    //加载yaml文件，有相同的名字，就进行更新
    for(auto& ite : all_nodes) {
        // WYZE_LOG_INFO(WYZE_LOG_ROOT()) << "---------------------";
        // WYZE_LOG_INFO(WYZE_LOG_ROOT()) << ite.first << " : " << ite.second; 
        std::string key = ite.first;
        if(key.empty())
            continue;

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);
        
        if(var) {
            if(ite.second.IsScalar()) {
                var->fromString(ite.second.Scalar());
            }
            else {
                std::stringstream ss;
                // WYZE_LOG_DEBUG(WYZE_LOG_ROOT()) << ss.str();
                ss << ite.second;
                var->fromString(ss.str());
            }
        }
    }
}

static std::map<std::string, uint64_t> s_file2modifytime;
static Mutex s_mutex;

void Config::LoadFromConfDir(const std::string& path, bool force)
{
    std::string absolute_path = EnvMgr::GetInstance()->getAbsolutePath(path);
    std::vector<std::string> files;
    FSUtil::ListAllFile(files, absolute_path, ".yml");

    for(auto& i : files) {
        {
            //TODO::这里只是时间判断，如果有md5会更好
            struct stat st;
            if(lstat(absolute_path.c_str(), &st) < 0)
                continue;

            Mutex::Lock lock(s_mutex);
            if(s_file2modifytime[i] == (uint64_t)st.st_mtime) 
                continue;

            s_file2modifytime[i] = st.st_mtime;
        }

        try {
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            WYZE_LOG_INFO(g_logger) << "LoadConfFile file="
                << i << " ok";
        }
        catch(...) {
            WYZE_LOG_ERROR(g_logger) << "LoadConfFile file="
                << i << " fail";
        }
    }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
{
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin();
            it != m.end(); ++it) {
        cb(it->second);
    }

}
}