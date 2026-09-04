#pragma once
#include <map>
#include <string>
#include <vector>

//! @brief Шина сообщений для обмена данными между вершинами
class MessageBus {
public:
    //! @brief Отправить сообщение в конкретную вершину
    void sendTo(unsigned int nodeId, const std::string &msg);
    //! @brief Отправить сообщение всем вершинам
    void broadcast(const std::string &msg, unsigned int senderId, const std::vector<unsigned int> &allNodeIds);

    //! @brief Получить и очистить очередь для указанной вершины
    std::vector<std::string> fetchFor(unsigned int nodeId);
    //! @brief Рассылка всех накопленных сообщений (вызывается в конце такта)
    void flush();

private:
    std::map<unsigned int, std::vector<std::string>> m_inboxes;
    struct PendingMsg {
        unsigned int target;
        std::string content;
    };
    std::vector<PendingMsg> m_pending;
};
