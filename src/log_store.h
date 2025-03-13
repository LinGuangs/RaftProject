#pragma once
#include<string>
#include<hiredis/hiredis.h>     // hiredis 库，用于与 Redis 通信

// LogStore 类：使用Redis 实现的持久化日志存储（仅存储任期和投票信息）
class LogStore{
public:
    LogStore(int nodeId);       // 构造函数，链接 Redis 并指定节点ID
    ~LogStore();                // 析构函数，断开 Redis 连接

    int getCurrentTerm();       // 获取当前保存的任期号（如果没有则返回0）
    int getVotedFor();          // 获取当前保存的投票给的候选人ID（如果没有则返回-1）
    void setCurrentTerm(int term);      // 持久化当前任期号
    void setVotedFor(int candidateId);  // 持久化当前投票给的候选人ID
private:
    int nodeId;                 // 节点ID，用于区分 Redis 中的键空间
    redisContext* redisCtx;     // Redis 连接上下文
};

