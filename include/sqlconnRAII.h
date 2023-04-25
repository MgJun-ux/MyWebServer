/**
 * @author:MgJun
 * @brief:利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能。
 * 这是一个RAII（资源获取即初始化）类，用于在构造函数中获取MySQL连接，并在析构函数中释放MySQL连接。
 * 在构造函数中，该类使用SqlConnPool的GetConn函数从连接池中获取MySQL连接，并将其存储在成员变量sql_中。
 * 在析构函数中，如果sql_不为空，则将MySQL连接通过调用SqlConnPool的FreeConn函数释放回连接池。
 * 通过这种方式，RAII类确保在其生命周期中，获取的MySQL连接在使用完成后被自动释放，避免了手动释放MySQL连接的麻烦和可能的遗漏
 * @date: 23/3/20
*/

#pragma once

#include "sqlconnpool.h"
/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnRAII{
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool){
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;

    }

    ~SqlConnRAII(){
        if(sql_){
            connpool_->FreeConn(sql_);
        }
    }

private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};