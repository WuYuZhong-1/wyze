#ifndef _WYZE_SERVLET_H_
#define _WYZE_SERVLET_H_

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "../thread.h"

namespace wyze {
namespace http {

class Servlet {
public:
    using ptr = std::shared_ptr<Servlet>;

    Servlet(const std::string& name)
        : m_name(name) { }
    virtual ~Servlet() {}

    virtual int32_t handle(HttpRequest::ptr req, 
            HttpResponse::ptr rsp, HttpSession::ptr session) = 0;
    const std::string& getName() const { return m_name; }
private:
    std::string m_name;
};

class FunctionServlet: public Servlet {
public:
    using ptr = std::shared_ptr<FunctionServlet>;
    using callback = std::function<int32_t (HttpRequest::ptr req,
            HttpResponse::ptr rsp, HttpSession::ptr session)>;
    
    FunctionServlet(FunctionServlet::callback cb);
    int32_t handle(HttpRequest::ptr req, HttpResponse::ptr rsp
                    ,HttpSession::ptr session) override;
private:
    callback m_cb;
};

class ServletDispatch : public Servlet {
public:
    using ptr = std::shared_ptr<ServletDispatch>;
    using RWMutexType = RWMutex;

    ServletDispatch();
    int32_t handle(HttpRequest::ptr req, HttpResponse::ptr rsp
                    ,HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);
    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    RWMutexType m_mutex;
    //uri(/wyze/xxx) -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    //uri(/wyze/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
    //默认servlet, 所有路径都没有匹配到时使用
    Servlet::ptr m_default;
};

class NotFoundServer : public Servlet {
public:
    using ptr = std::shared_ptr<NotFoundServer>;
    NotFoundServer();
    int32_t handle(HttpRequest::ptr req, HttpResponse::ptr rsp
                    ,HttpSession::ptr session) override;
};

}
}

#endif