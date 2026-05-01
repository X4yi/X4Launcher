#pragma once

#include "VersionParser.h"
#include "../instance/Instance.h"
#include "../instance/InstanceConfig.h"
#include <QStringList>
#include <QMap>

namespace x4 {
    class ArgumentResolver {
    public:
        struct LaunchArgs {
            QString javaPath;
            QStringList jvmArgs;
            QStringList gameArgs;
            QString mainClass;
            QString classpath;
            QString nativesDir;
        };

        static LaunchArgs resolve(const VersionInfo &version,
                                  const Instance &instance,
                                  const InstanceConfig &config,
                                  const QString &javaPath,
                                  const QStringList &classpathEntries,
                                  const QString &nativesDir,
                                  const QString &gameDir,
                                  const QString &assetsDir);

    private:
        static QString substitute(const QString &tmpl, const QMap<QString, QString> &vars);

        static QStringList substituteAll(const QStringList &templates, const QMap<QString, QString> &vars);
    };
}