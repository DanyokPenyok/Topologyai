#pragma once
#include <QDateTime>
#include <QDebug>
#include <QString>
#include <mutex>
#include <vector>
#include <functional>

//! @brief Лёгкий потокобезопасный логгер для имитационного моделирования
class Logger {
public:
    using LogCallback = std::function<void(const QString &)>;

    static void registerCallback(LogCallback cb) {
        std::lock_guard<std::mutex> lock(mutex());
        callbacks().push_back(cb);
    }

    static void info(const QString &msg) {
        QString formatted;
        std::vector<LogCallback> cbs;
        {
            std::lock_guard<std::mutex> lock(mutex());
            formatted = QString("[INFO] [%1] %2")
                    .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                    .arg(msg);
            qDebug().noquote() << formatted;
            cbs = callbacks();
        }
        triggerCallbacks(cbs, formatted);
    }

    static void event(unsigned int nodeId, const QString &type) {
        QString formatted;
        std::vector<LogCallback> cbs;
        {
            std::lock_guard<std::mutex> lock(mutex());
            formatted = QString("[EVENT] [%1] Node-%2: %3")
                    .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                    .arg(nodeId)
                    .arg(type);
            qDebug().noquote() << formatted;
            cbs = callbacks();
        }
        triggerCallbacks(cbs, formatted);
    }

    static void step(double time, double availability) {
        QString formatted;
        std::vector<LogCallback> cbs;
        {
            std::lock_guard<std::mutex> lock(mutex());
            formatted = QString("[STEP] T=%1 | Availability=%2")
                    .arg(time, 0, 'f', 1)
                    .arg(availability, 0, 'f', 3);
            qDebug().noquote() << formatted;
            cbs = callbacks();
        }
        triggerCallbacks(cbs, formatted);
    }

private:
    static std::mutex &mutex() {
        static std::mutex m;
        return m;
    }

    static std::vector<LogCallback> &callbacks() {
        static std::vector<LogCallback> list;
        return list;
    }

    // Called OUTSIDE the mutex lock to prevent deadlocks
    static void triggerCallbacks(const std::vector<LogCallback> &cbs, const QString &msg) {
        for (const auto &cb : cbs) {
            if (cb) {
                cb(msg);
            }
        }
    }
};
