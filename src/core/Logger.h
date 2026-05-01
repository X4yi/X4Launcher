#pragma once

#include "Types.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>

namespace x4 {
    class Logger {
    public:
        static Logger &instance();

        void init(const QString &logFilePath);

        void setLevel(LogLevel level) { level_ = level; }
        LogLevel level() const { return level_; }

        void log(LogLevel level, const QString &category, const QString &message);


        void debug(const QString &cat, const QString &msg) { log(LogLevel::Debug, cat, msg); }
        void info(const QString &cat, const QString &msg) { log(LogLevel::Info, cat, msg); }
        void warning(const QString &cat, const QString &msg) { log(LogLevel::Warning, cat, msg); }
        void error(const QString &cat, const QString &msg) { log(LogLevel::Error, cat, msg); }

    private:
        Logger() = default;

        static const char *levelStr(LogLevel l) {
            switch (l) {
                case LogLevel::Debug: return "DEBUG";
                case LogLevel::Info: return "INFO";
                case LogLevel::Warning: return "WARN";
                case LogLevel::Error: return "ERROR";
                case LogLevel::Fatal: return "FATAL";
            }
            return "???";
        }

        LogLevel level_ = LogLevel::Info;
        QFile file_;
        QTextStream stream_;
        QMutex mutex_;
    };


#define X4_LOG(lvl_, cat, msg) \
    do { if (static_cast<uint8_t>(lvl_) >= static_cast<uint8_t>(::x4::Logger::instance().level())) \
        ::x4::Logger::instance().log(lvl_, cat, msg); } while(0)

#define X4_DEBUG(cat, msg) X4_LOG(::x4::LogLevel::Debug, cat, msg)
#define X4_INFO(cat, msg)  X4_LOG(::x4::LogLevel::Info, cat, msg)
#define X4_WARN(cat, msg)  X4_LOG(::x4::LogLevel::Warning, cat, msg)
#define X4_ERROR(cat, msg) X4_LOG(::x4::LogLevel::Error, cat, msg)
}