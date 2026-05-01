#pragma once

#include <QApplication>
#include <QString>
#include <QColor>

namespace x4 {
    class Theme {
    public:
        static void apply(const QString &themeName);

        static void applyDark();

        static void applyLight();

        static void applyZen();


        struct Colors {
            static QColor accent() { return QColor(0x4F, 0xA8, 0xE2); }
            static QColor success() { return QColor(0x50, 0xC8, 0x78); }
            static QColor warning() { return QColor(0xE2, 0xB7, 0x4F); }
            static QColor error() { return QColor(0xE2, 0x5F, 0x5F); }
            static QColor running() { return QColor(0x50, 0xC8, 0x78); }


            static QColor logInfo() { return QColor(0x8B, 0xC3, 0xE8); }
            static QColor logWarn() { return QColor(0xE2, 0xB7, 0x4F); }
            static QColor logError() { return QColor(0xE2, 0x5F, 0x5F); }
            static QColor logDebug() { return QColor(0x88, 0x88, 0x88); }
            static QColor logTime() { return QColor(0x6B, 0x9B, 0xB8); }
        };
    };
}