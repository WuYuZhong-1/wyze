#include "mysqlconn_pool.h"
#include <json/json.h>
#include <fstream>
#include <iostream>

namespace wyze {

MysqlConnPool::MysqlConnPool()
{
    if(!parseJsonFile())
        return;

    //创建最小的连接数
    for(int i = 0; i < m_minSize; ++i) {
        addConnection();
    }
    std::cout << "queue size = " << m_mysqlConnQueue.size() << std::endl;

    producer.reset(new std::thread(&MysqlConnPool::produceConnection, this));
    recycler.reset(new std::thread(&MysqlConnPool::recycleConnection, this));
}

MysqlConnPool::~MysqlConnPool()
{
    isexit = true;
    m_getconn.notify_all();

    producer->join();
    recycler->join();
    //这里要解决断开连接的问题，疑问在容器中直接erase是否可以析构对象
    while(m_mysqlConnQueue.empty() == false) {
        MysqlConn* mysql = m_mysqlConnQueue.front();
        m_mysqlConnQueue.pop();
        delete mysql;
    }
}

MysqlConnPool* MysqlConnPool::getInstance()
{
    //使用静态的局部变量,线程安全的
    static MysqlConnPool connPool;
    return &connPool;
}

bool MysqlConnPool::parseJsonFile()
{
    std::ifstream ifs("/home/wuyz/learn/mysql_pool/dbconf.json");
    Json::Reader reader;
    Json::Value root;

    if(reader.parse(ifs, root) == false) {
        std::cout << "parse json error";
        return false;
    }

    if(root.isObject()) {
        m_ip = root["ip"].asString();
        m_port = root["port"].asUInt();
        m_user = root["userName"].asString();
        m_password = root["password"].asString();
        m_dbName = root["dbName"].asString();
        m_minSize = root["minSize"].asUInt();
        m_maxSize = root["maxSize"].asUInt();
        m_timeout = root["timeout"].asUInt();
        m_maxIdleTime = root["maxIdleTime"].asUInt();
        return true;
    }

    return false;
}

void MysqlConnPool::produceConnection()
{
    //数据库的使用是从队列中pop出来个消费者使用，
    //生产者的作用是判断当前的队列个数小于最小数，则进行生产连接mysql的类
    while(true) {

        std::unique_lock<std::mutex> locker(m_mutexQ);
        while( m_mysqlConnQueue.size() >= (size_t)m_minSize && isexit == false) {      //当前队列数大于最小连接池数，则进行等待，等待消费者唤醒
            m_getconn.wait(locker, [this](){    
                if(this->m_mysqlConnQueue.size() < (size_t)this->m_minSize)
                    return true;
                return false;
            });
        }
        if(isexit == true)
            return;
        addConnection();
        m_produce.notify_all();     //通知消费者
    }
}

void MysqlConnPool::recycleConnection()
{
    while(true) {

        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::unique_lock<std::mutex> locker(m_mutexQ);
        while( m_mysqlConnQueue.size() > (size_t)m_minSize ) {

            MysqlConn* mysql = m_mysqlConnQueue.front();
            if(mysql->getAliveTime() >= m_maxIdleTime) {

                m_mysqlConnQueue.pop();
                delete mysql;
            }
            else {
                break;
            }

            if( isexit == true)
                return;
        }

        if( isexit == true)
            return;
    }
}

void MysqlConnPool::addConnection()
{
    MysqlConn* mysql = new MysqlConn();
    if(mysql->connect(m_user, m_password, m_dbName, m_ip, m_port) == false) {
        std::cout << "connect mysql error "<< std::endl;
        delete mysql; 
        return;
    }
    mysql->refreshAliveTime();
    m_mysqlConnQueue.push(std::move(mysql));
}

std::shared_ptr<MysqlConn> MysqlConnPool::getConnection()
{
    std::unique_lock<std::mutex> locker(m_mutexQ);
    while( m_mysqlConnQueue.empty()) {

        m_getconn.notify_all();  //通知生产者，队列为空，需要生产   
        std::cv_status status= m_produce.wait_for(locker, std::chrono::milliseconds(m_timeout));    //等待生产者
        if( status == std::cv_status::timeout) {

            if(m_mysqlConnQueue.empty()) {
                // return nullptr;
                continue;
            }
                
        }
    }

    std::shared_ptr<MysqlConn> mysqlPtr(m_mysqlConnQueue.front(), [this](MysqlConn* ptr){
        // std::unique_lock<std::mutex> locker(this->m_mutexQ);
        std::lock_guard<std::mutex> locker(this->m_mutexQ);
        ptr->refreshAliveTime();
        this->m_mysqlConnQueue.push(ptr);
        this->m_produce.notify_all();       //回首连接池也是 生产，通知消费者
    });
    m_mysqlConnQueue.pop();
    //通知生产者我获取了连接
    m_getconn.notify_all();        //这里既唤醒生产者线程，有唤醒消费者线程，但是消费者线程唤醒后会继续等待
    
    return mysqlPtr;
}

}