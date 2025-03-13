#include "log_store.h"
#include <iostream>

LogStore::LogStore(int nodeId) : nodeId(nodeId), redisCtx(nullptr)
{
    // 连接到 Redis 服务器（默认地址 127.0.0.1:6379）
    redisCtx = redisConnect("127.0.0.1", 6379);
    if (redisCtx == nullptr || redisCtx->err)
    {
        if (redisCtx)
        {
            std::cerr << "Redis connection error:" << redisCtx->errstr << std::endl;
            // 如果连接失败，释放上下文以避免内存泄漏
            redisFree(redisCtx);
            redisCtx = nullptr;
        }
        else
        {
            std::cerr << "Redis connection error: can't allocate redis context\n";
        }
        // 没有退出程序，但后续需要检查 redisCtx 是否为 null（即无持久化可用）
    }
}

LogStore::~LogStore()
{
    if (redisCtx)
    {
        redisFree(redisCtx);
        redisCtx = nullptr;
    }
}

// 从Redis 获取 currentTerm（如果没有设置或出错则返回 0）
int LogStore::getCurrentTerm()
{
    if (!redisCtx)
    {
        return 0; // 如果没有可用的 Redix 连接，则返回默认值0
    }
    // 构建 HGET 命令，键为 "node:<nodeId>"，字段为 "currentTerm"
    std::string command = "HGET node:" + std::to_string(nodeId) + "currentTerm";
    redisReply *reply = (redisReply *)redisCommand(redisCtx, command.c_str());
    if (!reply)
    {
        std::cerr << "Redis HGET currentTerm failed." << std::endl;
        return 0;
    }
    int term = 0;
    if (reply->type == REDIS_REPLY_STRING)
    {
        term = std::atoi(reply->str); // 将返回的字符串转换为证书
    }
    // 如果返回类型是NIL 或其他类型，term将保持为0
    freeReplyObject(reply);
    return term;
}

// 从 Redis 获取 votdFor（如果没有设置或设置出错则返回 -1）
int LogStore::getVotedFor()
{
    if (!redisCtx)
    {
        return -1; // 如果没有可用的 Redis 连接，则返回默认值 -1
        std::string command = "HGET node:" + std::to_string(nodeId) + " votedFor";
        redisReply *reply = (redisReply *)redisCommand(redisCtx, command.c_str());
    }
    std::string command = "HGET node:" + std::to_string(nodeId) + " votedFor";
    redisReply *reply = (redisReply *)redisCommand(redisCtx, command.c_str());
    if (!reply)
    {
        std::cerr << "Redis HGET votedFor failed." << std::endl;
        return -1;
    }
    int vote = -1;
    if (reply->type == REDIS_REPLY_STRING)
    {
        vote = std::atoi(reply->str); // 将返回的字符串转换为整数
    }
    freeReplyObject(reply);
    return vote;
}

// 将 currentTerm 写入 Redis
void LogStore::setCurrentTerm(int term)
{
    if (!redisCtx)
    {
        return; // 如果没有 Redis 连接，则直接返回（无法持久化）
    }
    std::string command = "HSET node:" + std::to_string(nodeId) + " currentTerm " + std::to_string(term);
    redisReply *reply = (redisReply *)redisCommand(redisCtx, command.c_str());
    if (!reply)
    {
        std::cerr << "Redis HSET currentTerm failed." << std::endl;
        return;
    }
    freeReplyObject(reply);
}

// 将 votedFor 写入 Redis
void LogStore::setVotedFor(int candidateId)
{
    if (!redisCtx)
    {
        return; // 如果没有 Redis 连接，则无法持久化投票者信息，直接返回
    }
    std::string command;
    if (candidateId == -1)
    {
        // 若 candidateId 为 -1，表示没有投给任何候选人，用字符串 "none" 表示
        command = "HSET node:" + std::to_string(nodeId) + " votedFor none";
    }
    else
    {
        command = "HSET node:" + std::to_string(nodeId) + " votedFor " + std::to_string(candidateId);
    }
    redisReply *reply = (redisReply *)redisCommand(redisCtx, command.c_str());
    if (!reply)
    {
        std::cerr << "Redis HSET votedFor failed." << std::endl;
        return;
    }
    freeReplyObject(reply);
}

// TODO: 可以在此模块扩展日志条目的存储和读取功能，并实现日志压缩（Log Compaction）和快照（Snapshot）等高级特性
