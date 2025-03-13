#pragma once    // 防止头文件重复包含
#include <iostream>  // 输入输出流库，用于打印调试信息
#include <thread>    // 线程支持库，用于模拟节点线程
#include <chrono>    // 时钟库，用于实现超时计时
#include <mutex>     // 互斥锁，用于线程间同步访问共享数据
#include <atomic>    // 原子类型，用于线程安全地更新标志和计数
#include <vector>      // 向量容器，用于存储节点列表等
#include <cstdlib>     // C 标准库，包含 rand() 和 srand()
#include <ctime>       // C 时间库，用于获取当前时间以播种随机数
#include "log_store.h" // 日志存储模块头文件（使用 Redis 持久化 currentTerm 和 votedFor）
#include "network.h"   // 网络模块头文件（gRPC 通信或模拟）


// 定义 Raft 节点的三种状态
enum class RaftState{
    Follower,   // 跟随者状态
    Candidate,  // 候选者状态
    Leader,     // 领导者状态
};

// RaftNode 类：表示 Raft 协议中的一个节点，包含选举相关的核心逻辑
class RaftNode{
public:
    RaftNode(int id);       // 构造函数，初始化节点状态和持久化存储
    ~RaftNode();            // 析构函数

    void startElection();    // 启动选举（将本节点转换为候选人，并发起投票请求）

    // 接收来自其他候选人的投票请求（RequestVote RPC）
    // 参数：term - 候选人的任期号，candidateId - 
    // 候选人节点ID，lastLogIndex/lastLogTerm - 候选人最后日志条目的索引和任期（用于日志是否最新的判断）
    // 返回值：如果本节点同意投票给该候选人则返回true，否则返回false
    bool receiveVoteRequest(int term, int candidateId, int lastLogIndex, int lastLogTerm);

    void runElection();     // 运行选举超时检查的函数（通常在独立线程中运行）

    // Getter 函数：用于测试或外部获取节点状态
    int getId();            // 获得节点Id
    int getCurrentTerm();   // 获取当前任期号
    int getVotedFor();      // 获取当前任期内投票对象的Id
    RaftState getState();   // 获取节点当前状态（Follower/Candidate/Leader）

private:
    int id;                         // 节点唯一标识符（ID）
    std::atomic<int> currentTerm;   // 当前任期号（使用原子类型以支持多线程更新）
    std::atomic<int> votedFor;      // 当前任期内投票投给的候选人ID（-1 表示尚未投票）
    std::atomic<RaftState> state;    // 节点当前状态（（跟随者、候选人、领导者）
    std::mutex mtx;                 // 互斥锁，保护节点状态的更新
    LogStore logStore;              // 日志存储对象，用于持久化 currentTerm 和 votedFor等信息

    // 将节点转为跟随者状态，并更新任期号为 newTerm
    void becomeFollower(int newTerm);
};