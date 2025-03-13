#include "raft_node.h"

// 声明一个外部的原子标志，用于指示是否已经有节点当选为 Leader（用于模拟中停止其他选举）
extern std::atomic<bool> electionDone;

// 构造函数：初始化 RaftNode，设置初始化任期和投票对象，并加载持久化的值
RaftNode::RaftNode(int id)
    : id(id), currentTerm(0), votedFor(-1), state(RaftState::Follower), logStore(id)
{
    // 使用当前时间加节点ID作为随机种子，确保不同节点有不同随机超时时间
    std::srand(std::time(nullptr) + id);
    // 从持久化存储加载 currentTerm 和 votedFor，如果没有记录则使用默认值 （0 和 -1）
    currentTerm = logStore.getCurrentTerm();
    votedFor = logStore.getVotedFor();
    // 确保节点启动时状态为 Follower
    state = RaftState::Follower;
}

RaftNode::~RaftNode()
{
    // 析构函数，目前不需要 特殊操作，资源由各自成员（如 LogStore）管理
}

// becomeFollower：将节点转换为 Follower，并设置新的任期号
void RaftNode::becomeFollower(int newTerm)
{
    std::lock_guard<std::mutex> lock(mtx); // 加锁以防止状态并发修改
    state = RaftState::Follower;
    currentTerm = newTerm; // 更新当前任期号为更大的 newTerm
    votedFor = -1;         // 重置投票选择（当前任期内还未投票给任何人）
    // 将更新后的 currentTerm 和 重置的 votedFor 写入 持久化存储
    logStore.setCurrentTerm(newTerm);
    logStore.setVotedFor(-1);
}

// StartElection：发起一次领导人选举过程
void RaftNode::startElection()
{
    // 先获取锁以保证修改状态的一致性
    std::lock_guard<std::mutex> lock(mtx); // 加锁
    if (electionDone)
    {
        // 如果已经有节点当选为 Leader，则不再发起新的选举
        return;
    }
    // 增加当前任期号，表示开始一个新的任期选举
    int term = currentTerm + 1;
    currentTerm = term;
    // 将自身状态转为候选人
    state = RaftState::Candidate;
    // 在新的任期里首先投票给自己
    votedFor = id;
    // 将更新后的 currentTerm 和 投票选择持久化
    logStore.setCurrentTerm(term);
    logStore.setVotedFor(id);
    std::cout << "Node：" << id << "became Candidate" << term << std::endl;
    // 统计投票：先计算自己的票
    int voteCount = 1;
    // 获取集群中除自己以外所有其他节点ID列表
    std::vector<int> peers = Network::getOtherNodes(id);
    //  准备 RequestVote RPC 请求内容（包含候选人的任期和ID，以及最后日志索引和任期，这里 简化为0）
    RequestVoteRequest req;
    req.term = term;
    req.candidateId = id;
    req.lastLogIndex = 0;
    req.lastLogTerm = 0;
    // 向每一个其他节点发送投票请求
    for (int peerId : peers)
    {
        RequestVoteResponse resp = Network::sendRequestVote(peerId, req);
        if (resp.voteGranted)
        {
            voteCount++;
        }
        // 如果从响应中发现有i节点的任期号比自己更大，则需要认输并转为跟随者。
        if (resp.term > currentTerm)
        {
            std::cout << "Node :" << id << " founder a higher term "
                      << resp.term << " from node " << peerId << std::endl;
            return; // 终止当前选举过程
        }
    }
    // 所有投票发送完毕后，检查自己所获得的票数是否过半（大多数节点）
    if (voteCount > peers.size() / 2)
    {
        // 若赢得多数选票，则成为 Leader
        state = RaftState::Leader;
        std::cout << "Node " << id << " wins election for term " << term
                  << " with " << voteCount << " votes, becomes Leader" << std::endl;
        // 标记已经产生领导者，通知其他线程进一步选举
        electionDone = true;
        // 作为Leader，通常需要立即发送初始心跳（AppendEntries RPC）通知其他节点自己当选
        // 这里暂时未实现心跳逻辑，暂时省略，只做提示
    }
    else
    {
        // 未获得多数选票，当选失败，可能出现选票平局等情况
        std::cout << "Node " << id << " election for term " << term << " failed (votes = " << voteCount << ")" << std::endl;
        // 简化处理： 直接将状态重置为跟随者，等待下一次选举超时
        // 在实际的 Raft 算法中，节点将继续保持候选人状态或回退为跟随者，然后等待随机的下一次超时重新发起选举
        state = RaftState::Follower;
    }
}

// receiveVoteRequest：处理收到的 RequestVote 投票请求
bool RaftNode::receiveVoteRequest(int term, int candidateId, int lastLogINdex, int lastLogTerm)
{
    std::lock_guard<std::mutex> lock(mtx); // 加锁保护状态
    // 如果候选人的任期小于本节点的当前任期，则拒绝投票
    if (term < currentTerm)
    {
        return false;
    }
    // 如果候选人的任期比本节点当前任期更大，则更新本节点的任期并转换为跟随者
    if (term > currentTerm)
    {
        std::cout << "Node " << id << " received higher term " << term << "from candidate" << candidateId << ", updating term and becoming Follower" << std::endl;
        currentTerm = term;
        state = RaftState::Follower;
        votedFor = -1; // 重置之前的投票记录，因为进入了新的任期
        // 持久化更新后的状态
        logStore.setCurrentTerm(term);
        logStore.setVotedFor(-1);
    }
    // 检查是否已经在此任期投过票：如果尚未投票（votedFor == -1）或投票对象就是该候选人
    if (votedFor == -1 || votedFor == candidateId)
    {
        // （此处为了简化，不检查lastLogIndex 和 lastLogTerm，即不判断候选人日志是否比本节点新，默认候选人日志足够新）
        // 投票给请求的候选人
        votedFor = candidateId;
        /// 将投票结果持久化
        logStore.setVotedFor(candidateId);
        std::cout << "Node " << id << " votes for candidate " << candidateId << " for term " << term << std::endl;
        return true;
    }
    // 否则，如果已经投过票不是给该候选人，则拒绝再次投票
    return false;
}

// runElection：用于在后台线程中运行，等待选举超时并发起选举
void RaftNode::runElection()
{
    // 每个跟随者等待一个随机的选举超时时间，然后如果没有检测到领导人则发起选举
    // 在此 简化为只进行一次选举尝试
    // 随机等待 150 到 300 毫秒
    int timeout_ms = 150 + std::rand() % 150;
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    // 超时后，如果还没有领导产生，并且本节点不是 Leader，则启动选举
    if (!electionDone && state != RaftState::Leader)
    {
        startElection();
    }
    // 在完整实现中，如果选举没有成功产生领导，应重新进入下一轮选举计时
    // 另外，Leader 在当选后会定期发送心跳（AppendEntries）以防止其他节点再次发起选举，这里的实现未包括心跳
}

int RaftNode::getId()
{
    return id;
}
int RaftNode::getCurrentTerm()
{
    return currentTerm;
}
int RaftNode::getVotedFor()
{
    return votedFor;
}
RaftState RaftNode::getState()
{
    return state;
}
