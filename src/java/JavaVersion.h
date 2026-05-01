#pragma once

#include <QString>

namespace x4 {
    struct JavaVersion {
        int major = 0;
        int minor = 0;
        int patch = 0;
        QString path;
        QString arch;
        bool isManaged = false;

        bool isValid() const { return major > 0 && !path.isEmpty(); }

        bool operator<(const JavaVersion &o) const {
            if (major != o.major) return major < o.major;
            if (minor != o.minor) return minor < o.minor;
            return patch < o.patch;
        }

        bool operator>=(const JavaVersion &o) const { return !(*this < o); }

        QString versionString() const {
            return QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
        }
    };
}