#pragma once  // 防止头文件重复包含
#include <unordered_map>  // 哈希容器，用于存储节点ID到节点对象指针的映射
#include <vector>         // 向量容器，用于存储节点ID列表
#include <mutex>          // 互斥锁头文件，用于 std::mutex

// 前向声明 RaftNode 类，避免头文件循环依赖
class RaftNode;

// 定义 RequestVote RPC 请求和响应的数据结构（类似于 proto 文件中的定义）
struct RequestVoteRequest{
    int term;         // 候选人任期号
    int candidateId;  // 候选人节点的 ID
    int lastLogIndex; // 候选人最后日志条目的索引（用于判断日志的新旧程度）
    int lastLogTerm;  // 候选人最后日志条目的任期号
};

struct RequestVoteResponse{
    int term;           // 响应方（投票者）的当前任期号（可能更新了候选人的任期）
    bool voteGranted;   // 是否同意投票给候选人
};

// Network 类：模拟网络通信（或抽象 gRPC调用）
class Network{
public:
    // 注册节点，将节点指针加入网络映射，以便通过 ID 查找
    static void registerNode(int id, RaftNode* node);
    // 取消注册节点（当前示例未用到，提供完整性）
    static void unregisterNode(int id);
    // 获取集群中除指定节点以外的所有节点的 ID 列表
    static std::vector<int> getOtherNodes(int selfId);
    // 模拟发送 RequestVote RPC 给目标节点（通过直接调用节点的 receiveVoteRequest 来模拟网络）
    static RequestVoteResponse sendRequestVote(int targetId, const RequestVoteRequest& request);
private:
    static std::unordered_map<int, RaftNode*> nodes;    // 节点ID到节点对象的全局映射表
    static std::mutex netMutex;                         // 保护 nodes 映射的互斥锁
};
