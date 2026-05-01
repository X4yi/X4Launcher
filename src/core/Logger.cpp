#include "Logger.h"

namespace x4 {
    Logger &Logger::instance() {
        static Logger logger;
        return logger;
    }

    void Logger::init(const QString &logFilePath) {
        QMutexLocker lock(&mutex_);
        if (file_.isOpen()) file_.close();

        file_.setFileName(logFilePath);
        if (file_.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            stream_.setDevice(&file_);
        }
    }

    void Logger::log(LogLevel level, const QString &category, const QString &message) {
        if (static_cast<uint8_t>(level) < static_cast<uint8_t>(level_))
            return;

        QString line = QStringLiteral("[%1] [%2] [%3] %4")
                .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                     QString::fromLatin1(levelStr(level)),
                     category,
                     message);

        QMutexLocker lock(&mutex_);


        if (level >= LogLevel::Warning)
            qWarning().noquote() << line;
        else
            qInfo().noquote() << line;


        if (stream_.device()) {
            stream_ << line << '\n';
            stream_.flush();
        }
    }
}