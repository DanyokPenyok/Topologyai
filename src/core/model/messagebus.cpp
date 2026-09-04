#include "messagebus.h"

void MessageBus::sendTo(unsigned int nodeId, const std::string &msg) { m_pending.push_back({nodeId, msg}); }

void MessageBus::broadcast(const std::string &msg, unsigned int senderId, const std::vector<unsigned int> &allNodeIds) {
    for (unsigned int nodeId: allNodeIds) {
        if (nodeId != senderId) {
            sendTo(nodeId, msg);
        }
    }
}

std::vector<std::string> MessageBus::fetchFor(unsigned int nodeId) {
    auto it = m_inboxes.find(nodeId);
    if (it == m_inboxes.end())
        return {};
    auto msgs = std::move(it->second);
    m_inboxes.erase(it);
    return msgs;
}

void MessageBus::flush() {
    for (const auto &pending: m_pending) {
        m_inboxes[pending.target].push_back(pending.content);
    }
    m_pending.clear();
}
