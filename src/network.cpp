#include "network.h"
#include "raft_node.h"
#include <iostream>

// 初始化 NetWork 类的静态成员变量
std::unordered_map<int, RaftNode *> Network::nodes; // 节点映射表的定义
std::mutex Network::netMutex;                       // 静态互斥锁的定义

void Network::registerNode(int id, RaftNode *node)
{
    std::lock_guard<std::mutex> lock(netMutex);
    nodes[id] = node; // 将节点指针按ID存储
}

void Network::unregisterNode(int id){
    std::lock_guard<std::mutex> lock(netMutex);
    nodes.erase(id);    // 从映射表中移除该节点
}

std::vector<int> Network::getOtherNodes(int selfId){
    std::lock_guard<std::mutex> lock(netMutex);
    std::vector<int> peers;
    for(auto& entry : nodes){
        if(entry.first != selfId){
            peers.push_back(entry.first);
        }
    }
    return peers;
}

RequestVoteResponse Network::sendRequestVote(int targetId, const RequestVoteRequest& request){
    RequestVoteResponse resp;
    resp.term = request.term;       // 默认先将响应中的任期设为请求的任期
    resp.voteGranted = false;       // 默认未获得投票
    // 查找目标节点
    std::lock_guard<std::mutex> lock(netMutex);
    auto it = nodes.find(targetId);
    if(it == nodes.end()){
        std::cerr << "Network : NOde " << targetId << " not found! " << std::endl;
        return resp;        // 如果目标节点不存在，返回拒绝投票（voteGranted=false）
    }
    RaftNode* targetNode = it->second;
    // 模拟通过网络进行 RPC：直接调用目标节点的 receiveVoteRequest 方法
    bool vote = targetNode->receiveVoteRequest(request.term, request.candidateId, request.lastLogIndex, request.lastLogTerm);
    resp.voteGranted = vote;
    // 将响应者（目标节点）的当前任期写入响应中，以便候选人更新自己的任期（如果需要）
    resp.term = targetNode->getCurrentTerm();
    return resp;
}
