#include "servlet.h"
#include <fnmatch.h>

namespace wyze {
namespace http {

FunctionServlet::FunctionServlet(FunctionServlet::callback cb)
    : Servlet("FunctonServlet")
    , m_cb(cb)
{
}

int32_t FunctionServlet::handle(HttpRequest::ptr req, HttpResponse::ptr rsp
                                ,HttpSession::ptr session)
{
    return m_cb(req, rsp, session);
}

ServletDispatch::ServletDispatch()
    : Servlet("ServletDispatch")
{
    m_default.reset(new NotFoundServer);
}

int32_t ServletDispatch::handle(HttpRequest::ptr req, HttpResponse::ptr rsp
                                ,HttpSession::ptr session)
{
    auto slt = getMatchedServlet(req->getPath());
    if(slt)
        return slt->handle(req, rsp, session);
    return -1;
}

void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt)
{
    RWMutexType::WriteLock wlock(m_mutex);
    m_datas[uri] = slt;
}

void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::callback cb)
{
    RWMutexType::WriteLock wlock(m_mutex);
    m_datas[uri].reset(new FunctionServlet(cb));
}

void ServletDispatch::addGlobServlet(const std::string& uri, Servlet::ptr slt)
{
    RWMutexType::WriteLock wlock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.emplace_back(uri, slt);
}

void ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::callback cb)
{
    addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispatch::delServlet(const std::string& uri)
{
    RWMutexType::WriteLock wlock(m_mutex);
    m_datas.erase(uri);
}

void ServletDispatch::delGlobServlet(const std::string& uri)
{
    RWMutexType::WriteLock wlock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

Servlet::ptr ServletDispatch::getServlet(const std::string& uri)
{
    RWMutexType::ReadLock rlock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri)
{
    RWMutexType::ReadLock rlock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri)
            return it->second;
    }
    return nullptr;
}

Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri)
{
    RWMutexType::ReadLock rlock(m_mutex);
    auto mit = m_datas.find(uri);
    if(mit != m_datas.end()) 
        return mit->second;

    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if( !fnmatch(it->first.c_str(), uri.c_str(), 0))
            return it->second;
    }

    return m_default;
}

NotFoundServer::NotFoundServer()
    : Servlet("NotFoundServlet")
{
}

int32_t NotFoundServer::handle(HttpRequest::ptr req, HttpResponse::ptr rsp
                                ,HttpSession::ptr session)
{
    char buf[1024] = {0};

    snprintf(buf, sizeof(buf), "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>%s</center></body></html>", rsp->getHeader("Server").c_str());
    rsp->setStatus(HttpStatus::NOT_FOUND);
    rsp->setHeader("Content-Type", "text/html");
    rsp->setBody(buf); 
    
    return 0;
}


}
}