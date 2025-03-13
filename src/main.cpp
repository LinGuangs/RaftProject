#include <iostream>
#include <thread>
#include <vector>
#include "raft_node.h" // Raft 节点类定义
#include "network.h"   // 网络模块定义

// 定义一个全局的原子布尔变量，指示选举是否已经结束（已有 Leader 产生）
std::atomic<bool> electionDone(false);

int main()
{
    // i定义集群中的e节点n数量，例如 3 个节点
    const int clusterSize = 3;
    std::vector<RaftNode *> nodes;
    nodes.reserve(clusterSize); // 利用reserve确定vector的容量

    // 创建多个 RaftNode 实例（比如 3 个节点，ID从 1 开始）
    for (int i = 1; i <= clusterSize; ++i)
    {
        RaftNode *node = new RaftNode(i);
        nodes.push_back(node);
        // 注册节点到 Network，以便让它们可以互相发送 RPC 请求
        Network::registerNode(i, node);
    }

    // 为每个节点启动一个线程，运行其选举超时逻辑，模拟节点并发运行
    std::vector<std::thread> threads;
    for (int i = 0; i < clusterSize; ++i)
    {
        // 将 RaftNode::runElection 作为线程函数，让每个节点各自在独立线程中运行
        // 等效于开辟 一个线程，执行 nodes[i]->runElection(); 函数
        threads.emplace_back(&RaftNode::runElection, nodes[i]);
    }
    // 等待所有节点线程完成（即选举过程结束）
    for (auto &t : threads)
    {
        t.join();
    }
    // 选举结束后，输出每个节点的状态（谁是 Leader， 谁是 Follower）
    for (auto node : nodes)
    {
        std::string role;
        switch (node->getState())
        {
        case RaftState::Follower:
            role = "Follower";
            break;
        case RaftState::Candidate:
            role = "Candidate";
            break;
        case RaftState::Leader:
            role = "Leader";
            break;
        }
        std::cout << "Node " << node->getId()
                  << ": Term=" << node->getCurrentTerm()
                  << ", VotedFor=" << node->getVotedFor()
                  << ", State=" << role << std::endl;
    }
    // 清理分配的节点对象
    for (auto node : nodes)
    {
        delete node;
    }
    return 0;
}
