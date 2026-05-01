#pragma once

#include "../core/Types.h"
#include <QJsonObject>
#include <QVector>

namespace x4 {
    struct VersionInfo {
        QString id;
        QString mainClass;
        QString assetsVersion;
        QUrl assetIndexUrl;
        QString assetIndexSha1;
        QVector<LibraryInfo> libraries;
        QStringList jvmArgTemplates;
        QStringList gameArgTemplates;
        QString inheritsFrom;
        QString type;
        QUrl clientJarUrl;
        QString clientJarSha1;
        qint64 clientJarSize = 0;
    };

    class VersionParser {
    public:
        static Result<VersionInfo> parse(const QByteArray &json);

        static VersionInfo merge(const VersionInfo &parent, const VersionInfo &child);

    private:
        static QVector<LibraryInfo> parseLibraries(const QJsonArray &arr);

        static QStringList parseArguments(const QJsonValue &val);

        static bool evaluateRules(const QJsonArray &rules);

        static bool matchesCurrentOS(const QJsonObject &osObj);
    };
}