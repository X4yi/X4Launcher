#include "JavaManager.h"
#include "../core/SettingsManager.h"
#include "../core/Logger.h"
#include "../core/Types.h"

namespace x4 {
    JavaManager::JavaManager(QObject *parent) : QObject(parent) {
        connect(&detector_, &JavaDetector::detected, this, [this](const QVector<JavaVersion> &) {
            emit detectionFinished();
        });
        connect(&provisioner_, &JavaProvisioner::progress, this, &JavaManager::provisionProgress);
        connect(&provisioner_, &JavaProvisioner::finished, this,
                [this](bool success, const JavaVersion &, const QString &error) {
                    if (success) {
                        detector_.detectAll();
                    }
                    emit provisionFinished(success, error);
                });
        // React to global settings changes: re-run detection when java settings change
        connect(&SettingsManager::instance(), &SettingsManager::settingChanged, this,
                [this](const QString &key) {
                    if (key.startsWith(QStringLiteral("java/"))) {
                        detector_.detectAll();
                    }
                });
    }

    void JavaManager::init() {
        detector_.detectAll();
    }

    int JavaManager::requiredJavaMajor(const QString &mcVersion) {
        auto v = Version::fromString(mcVersion);

        if (mcVersion.contains('w') || mcVersion.contains(QStringLiteral("snapshot"))) {
            return 21;
        }

        if (v.major == 1) {
            if (v.minor >= 21) return 21;
            if (v.minor >= 18) return 17;
            if (v.minor >= 17) return 16;
            return 8;
        }

        if (v.major >= 23) return 25;

        return 21;
    }

    JavaVersion JavaManager::findJavaForMC(const QString &mcVersion) const {
        int required = requiredJavaMajor(mcVersion);

        QString manualPath = SettingsManager::instance().defaultJavaPath();
        if (!manualPath.isEmpty()) {
            auto manual = JavaDetector::probeJavaBinary(manualPath);
            if (manual.isValid() && manual.major >= required) {
                return manual;
            }
        }

        const auto &javas = detector_.results();

        for (const auto &j: javas) {
            if (j.major == required) return j;
        }

        JavaVersion best;
        for (const auto &j: javas) {
            if (j.major >= required) {
                if (!best.isValid() || j.major < best.major) {
                    best = j;
                }
            }
        }

        return best;
    }

    void JavaManager::provisionJava(int majorVersion) {
        provisioner_.provision(majorVersion);
    }

    void JavaManager::refresh() {
        detector_.detectAll();
    }
} // namespace x4