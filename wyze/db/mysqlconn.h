#ifndef _WYZE_MYSQLCONN_H_
#define _WYZE_MYSQLCONN_H_

#include <string>
#include <mysql/mysql.h>
#include <chrono>
#include "../thread.h"
#include <list>

namespace wyze {

class MysqlConn {
public:
    using ptr = std::shared_ptr<MysqlConn>;
    //初始化数据库连接
    MysqlConn();
    //释放数据库连接
    ~MysqlConn();
    //连接数据库
    bool connect(const std::string& user, const std::string& password, const std::string& dbName,
                    const std::string& ip, unsigned short port = 3306);
    //更新数据库 insert, update, delete
    bool update(const std::string& sql);
    //查询数据库
    bool query(const std::string& sql);
    //遍历得到的结果集合
    bool next();
    //得到结果集合中的字段值
    std::string value(int index);
    //字段的个数
    int fields();
    //事物操作
    bool transaction();
    //事物提交
    bool commit();
    //事物回滚
    bool rollback();
    //刷新空闲时间点
    void refreshAliveTime();
    //计算连接存活的总时长
    long long getAliveTime();
private:
    void freeResult();

private:
    MYSQL* m_mysql = nullptr;
    MYSQL_RES* m_result = nullptr;
    MYSQL_ROW m_row = nullptr;
    std::chrono::steady_clock::time_point  m_aliveTime;
};

class MysqlConnPool {
public:
    using ptr = std::shared_ptr<MysqlConnPool>;
    using MutexType = RWMutex;
    MysqlConnPool();
    MysqlConn::ptr getConnection();

private:
    std::string m_ip;               //ip地址
    uint16_t m_port;                //端口号
    uint32_t m_maxSize;             //最大连接数
    uint32_t m_maxAliveTime;        //存活的最大时间
    uint32_t m_maxQueryTimes;       //最大查询次数

    MutexType m_mutex;              
    std::list<MysqlConn*> m_conns;
    std::atomic<uint32_t> m_totals; //总共创建的连接数

};

}

#endif // _MYSQLCONN_H_