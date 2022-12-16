#ifndef __WYZE_CONFIG_H__
#define __WYZE_CONFIG_H__

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <forward_list>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "log.h"
#include "thread.h"

namespace wyze {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string description = "")
        : m_name(name), m_description(description){
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() { }

    const std::string& getName() const { return m_name; }
    const std::string& getDescription() const { return m_description; } 

    virtual std::string toString() = 0;                         //将里面的值转为std::string
    virtual bool fromString(const std::string& val) = 0;        // 根据std::string 设置里面的值
    virtual std::string getTypeName() const  = 0;
protected:
    std::string m_name;
    std::string m_description;
};

//F from_type, T to_type
template <class F, class T>
class LexicalCast {
public:
    T operator()(const F& val) {
        return boost::lexical_cast<T>(val);
    }
};

template <class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string& val) {
        typename std::vector<T> vec;
        YAML::Node node = YAML::Load(val);
        std::stringstream ss;
        for(size_t i =0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));      //push_back的方法是把std::string 转为 sequence
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string& val) {
        YAML::Node node = YAML::Load(val);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <class T>
class LexicalCast<std::string, std::forward_list<T>> {
public:
    std::forward_list<T> operator()(const std::string& val) {
        YAML::Node node = YAML::Load(val);
        typename std::forward_list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_front(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::forward_list<T>, std::string> {
public:
    std::string operator()(const std::forward_list<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template <class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string& val) {
        YAML::Node node = YAML::Load(val);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string& val) {
        YAML::Node node = YAML::Load(val);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string& val) {
        YAML::Node node = YAML::Load(val);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for(auto ite = node.begin();
                ite != node.end(); ++ite) {
            ss.str("");
            ss << ite->second;
            vec.insert(std::make_pair(ite->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string& val) {
        YAML::Node node = YAML::Load(val);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for(auto ite = node.begin();
                ite != node.end(); ++ite) {
            ss.str("");
            ss << ite->second;
            vec.insert(std::make_pair(ite->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node;
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// FromStr -> T operator()(const std::string&)
// ToStr std::string operator()(const T&)
template< typename T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar: public ConfigVarBase {
public:
    using RWMutexType = RWMutex;
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T& old_val, const T& new_val)> on_change_cb;
    ConfigVar(const std::string& name, const T& default_val, const std::string& description = "")
        : ConfigVarBase(name, description), m_val(default_val) { }
    
    virtual std::string toString() override {
        try {
            //return boost::lexical_cast<std::string>(m_val);
            RWMutexType::WriteLock lock(m_mutex);
            return ToStr()(m_val);
        }
        catch( std::exception& e) {    
            WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "ConfigVar::toString() exception:" << e.what()
                                            << " convert: " << typeid(m_val).name() << " to std::string";
        }
        return "";

    }
    virtual bool fromString(const std::string& val) override {
        try {
            //m_val = boost::lexical_cast<T>(val);
            setVal(FromStr()(val));
            return true;
        }
        catch(std::exception& e) {
            WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "ConfigVar::fromString() exception:" << e.what()
                                            << " convert:  std::string to " << typeid(m_val).name()
                                            << " - " << val ;
        }
        return false;
    }

    const T getValue() { 
        RWMutexType::ReadLock lock(m_mutex);
        return m_val; 
    }
    
    void setVal(const T& v) { 
        {
            RWMutexType::ReadLock lock(m_mutex);
            if( v == m_val )
                return ;
            for(auto i : m_cbs) {
                i.second(m_val, v);
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        m_val = v;
    }
    // const 要写在 override 后面， 因为 overrid 是检查，不是修饰函数
    virtual std::string getTypeName() const override { return typeid(T).name(); }
    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }
    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }
    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }
    on_change_cb getListener(uint64_t key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto i = m_cbs.find(key);
        return i == m_cbs.end() ? nullptr : i->second;
    }
private:
    RWMutexType m_mutex;
    T m_val;
    //function 不能进行 对比操作，不能进行删除需要对应的标记进行删除
    std::map<uint64_t, on_change_cb> m_cbs;
};

class Config {
public:
    typedef std::shared_ptr<Config> ptr;
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    using RWMutexType = RWMutex;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, 
            const T& default_val, const std::string& description = "") {
        
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        
        RWMutexType::WriteLock lock(GetMutex());
        auto ite = GetDatas().find(lower_name);

        //找到了相同的名字
        if(ite != GetDatas().end()) {
            //判断类型是否相同
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(ite->second);  //当向下转型出现问题，该指针为nullptr
            if(tmp) {
                WYZE_LOG_INFO(WYZE_LOG_ROOT()) << "Lookup name = " << name << " exists";
                return tmp;
            }
            else {
                WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "Lookup name = " << name << " exists but type not "
                                    << typeid(T).name() << " real_type = " << ite->second->getTypeName()
                                    << " " << ite->second->toString();
                return nullptr; 
            }
        }

        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") 
            != std::string::npos )  {     // std::string::npos 没有位置的意思
            WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "lookup name invaild " << name;
            throw std::invalid_argument(name);  //抛出异常
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_val, description));
        GetDatas()[name] = v;

        return v;
    }
    
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end())
            return nullptr;
        //智能指针向下转型
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static ConfigVarBase::ptr LookupBase(const std::string& name);
    static void LoadFromYaml(const YAML::Node& root);
    static void LoadFromConfDir(const std::string& path, bool force = false);
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);
private:
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }

    static RWMutexType& GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};

}

#endif //__WYZE_CONFIG_H__