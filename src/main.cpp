#include "core/PathManager.h"
#include "core/SettingsManager.h"
#include "core/Logger.h"
#include "network/HttpClient.h"
#include "java/JavaManager.h"
#include "cache/CacheManager.h"
#include "instance/InstanceManager.h"
#include "launch/VersionManifest.h"
#include "launch/LaunchEngine.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "core/Translator.h"
#include "core/ErrorHandler.h"
#include "core/Types.h"

#include <QApplication>
#include <QPixmapCache>
#include <QTimer>

#if defined(__GLIBC__)
#include <malloc.h>
#endif

int main(int argc, char* argv[]) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("X4Launcher"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("X4Launcher"));

    x4::PathManager::instance().init();

    x4::Logger::instance().init(
        x4::PathManager::instance().logsDir() + QStringLiteral("/x4launcher.log"));

#ifdef QT_DEBUG
    x4::Logger::instance().setLevel(x4::LogLevel::Debug);
#endif

    X4_INFO("Main", QStringLiteral("X4Launcher starting..."));
    X4_INFO("Main", QStringLiteral("Path base dir: %1").arg(x4::PathManager::instance().baseDir()));
    X4_INFO("Main", QStringLiteral("Path mode: Native"));

    x4::Translator::instance().setLanguage(x4::SettingsManager::instance().language());
    x4::ErrorHandler::instance().initDefaults();

    const bool zenStartup = x4::SettingsManager::instance().zenMode();
    if (zenStartup) {
        x4::Theme::applyZen();
        QPixmapCache::setCacheLimit(0);  // Disable pixmap cache for minimal RAM usage
    } else {
        x4::Theme::apply(x4::SettingsManager::instance().theme());
        QPixmapCache::setCacheLimit(2048);
    }

    QObject::connect(&x4::SettingsManager::instance(), &x4::SettingsManager::settingChanged,
            [](const QString& key) {
                if (key == QStringLiteral("ui/theme") && !x4::SettingsManager::instance().zenMode()) {
                    x4::Theme::apply(x4::SettingsManager::instance().theme());
                }
                if (key == QStringLiteral("ui/zenMode")) {
                    if (x4::SettingsManager::instance().zenMode()) {
                        x4::Theme::applyZen();
                        QPixmapCache::setCacheLimit(0);  // Disable pixmap cache for minimal RAM usage
                    } else {
                        x4::Theme::apply(x4::SettingsManager::instance().theme());
                        QPixmapCache::setCacheLimit(2048);
                    }
                }
                if (key == QStringLiteral("ui/language")) {
                    x4::Translator::instance().setLanguage(x4::SettingsManager::instance().language());
                }
                if (key == QStringLiteral("system/logLevel")) {
                    x4::Logger::instance().setLevel(static_cast<x4::LogLevel>(x4::SettingsManager::instance().logLevel()));
                }
            });
    
    x4::CacheManager cacheMgr;
    x4::JavaManager javaMgr;
    x4::InstanceManager instanceMgr;
    x4::VersionManifest manifest;
    x4::LaunchEngine launchEngine(&instanceMgr, &cacheMgr, &javaMgr, &manifest);

    javaMgr.init();
    manifest.fetch();
    
    #if defined(__GLIBC__)
    malloc_trim(0);
    #endif

    x4::MainWindow window(&instanceMgr, &cacheMgr, &javaMgr, &manifest, &launchEngine);
    QObject::connect(&x4::SettingsManager::instance(), &x4::SettingsManager::settingChanged,
            &window, [&window](const QString& key) {
                if (key == QStringLiteral("ui/zenMode")) {
                    window.applyZenMode(x4::SettingsManager::instance().zenMode());
                }
            });
    window.show();

    X4_INFO("Main", QStringLiteral("Startup complete"));

    QTimer::singleShot(5000, []() {
#if defined(__GLIBC__)
        malloc_trim(0);
#endif
    });

    return app.exec();
}
