#include "mysqlconn.h"
#include <iostream>

namespace wyze {

MysqlConn::MysqlConn()
{
    m_mysql = mysql_init(nullptr);
    if(m_mysql == nullptr) {
        std::cout << "mysql_init() error\n";
        return;
    }

    int ret = mysql_set_character_set(m_mysql, "utf8");
    if(ret != 0) {
        std::cout << "mysql_set_character_set() error: " << mysql_error(m_mysql);
    }
}

MysqlConn::~MysqlConn()
{
    freeResult();

    if(m_mysql != nullptr) 
        mysql_close(m_mysql);
}

bool MysqlConn::connect(const std::string& user, const std::string &password,
                        const std::string& dbName, const std::string& ip,
                        unsigned short port)
{
    if(m_mysql == nullptr) 
        return false;

    MYSQL* ptr = mysql_real_connect(m_mysql, ip.c_str(), user.c_str(), password.c_str(),
                                        dbName.c_str(), port, nullptr, 0);
    
    return ptr == m_mysql ? true: false;
}

bool MysqlConn::update(const std::string& sql)
{
    if(m_mysql == nullptr) 
        return false;

    int ret = mysql_query(m_mysql, sql.c_str());
    return ret == 0 ? true: false;
}

bool MysqlConn::query(const std::string& sql)
{
    if(m_mysql == nullptr) 
        return false;

    //释放查询结果集
    freeResult();

    int ret = mysql_query(m_mysql, sql.c_str());
    if(ret != 0) {
        return false;
    }

    m_result = mysql_store_result(m_mysql);

    return m_result == nullptr ? false: true;
}

bool MysqlConn::next()
{
    if(m_result == nullptr)
        return false;

    m_row = mysql_fetch_row(m_result);

    return m_row != nullptr ? true : false;
}

std::string MysqlConn::value(int index)
{
    if(m_row == nullptr || m_result == nullptr)
        return std::string();

    int fields = mysql_num_fields(m_result);
    if(index >= fields || index < 0)
        return std::string();
    
    //返回当前行的各个列的长度
    unsigned long* len = mysql_fetch_lengths(m_result);

    return std::string(m_row[index], len[index]);
}

bool MysqlConn::transaction()
{
    if(m_mysql == nullptr) 
        return false;

    int ret = mysql_autocommit(m_mysql, false);
    return ret == 0 ? true: false;
}

bool MysqlConn::commit()
{
    if(m_mysql == nullptr) 
        return false;

    int ret = mysql_commit(m_mysql);
    return ret == 0 ? true: false;
}

bool MysqlConn::rollback()
{
    if(m_mysql == nullptr) 
        return false;

    int ret = mysql_rollback(m_mysql);
    return ret == 0 ? true: false;
}

void MysqlConn::freeResult()
{
    if(m_result != nullptr) {
        mysql_free_result(m_result);
        m_result = nullptr;
    }
}

int MysqlConn::fields()
{
    if(m_result == nullptr)
        return 0;

    return mysql_num_fields(m_result);
}

void MysqlConn::refreshAliveTime()
{
    m_aliveTime = std::chrono::steady_clock::now();
}

long long MysqlConn::getAliveTime()
{
    std::chrono::nanoseconds res = std::chrono::steady_clock::now() - m_aliveTime;
    std::chrono::milliseconds millisec = std::chrono::duration_cast<std::chrono::milliseconds>(res);
    return millisec.count();
}





}