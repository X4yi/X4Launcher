#pragma once

#include <QString>
#include <QDateTime>
#include <variant>

namespace x4 {


template<typename T>
class Result {
public:
    static Result ok(T value) { return Result(std::move(value)); }
    static Result err(QString error) { return Result(std::move(error)); }

    bool isOk() const { return std::holds_alternative<T>(data_); }
    bool isErr() const { return !isOk(); }

    const T& value() const { return std::get<T>(data_); }
    T& value() { return std::get<T>(data_); }
    T takeValue() { return std::move(std::get<T>(data_)); }

    const QString& error() const { return std::get<QString>(data_); }

private:
    explicit Result(T value) : data_(std::move(value)) {}
    explicit Result(QString error) : data_(std::move(error)) {}
    std::variant<T, QString> data_;
};


template<>
class Result<void> {
public:
    static Result ok() { return Result(true); }
    static Result err(QString error) { return Result(std::move(error)); }

    bool isOk() const { return ok_; }
    bool isErr() const { return !ok_; }
    const QString& error() const { return error_; }

private:
    explicit Result(bool) : ok_(true) {}
    explicit Result(QString error) : ok_(false), error_(std::move(error)) {}
    bool ok_ = false;
    QString error_;
};


enum class ModLoaderType : uint8_t {
    Vanilla = 0,
    Fabric,
    Forge,
    NeoForge,
    Quilt
};

inline QString modLoaderName(ModLoaderType t) {
    switch (t) {
        case ModLoaderType::Vanilla:  return QStringLiteral("Vanilla");
        case ModLoaderType::Fabric:   return QStringLiteral("Fabric");
        case ModLoaderType::Forge:    return QStringLiteral("Forge");
        case ModLoaderType::NeoForge: return QStringLiteral("NeoForge");
        case ModLoaderType::Quilt:    return QStringLiteral("Quilt");
    }
    return QStringLiteral("Unknown");
}

enum class InstanceState : uint8_t {
    Ready = 0,
    Running,
    NeedsSetup,
    Downloading,
    Error
};

enum class LogLevel : uint8_t {
    Debug = 0,
    Info,
    Warning,
    Error,
    Fatal
};


struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;

    bool operator<(const Version& o) const {
        if (major != o.major) return major < o.major;
        if (minor != o.minor) return minor < o.minor;
        return patch < o.patch;
    }
    bool operator==(const Version& o) const {
        return major == o.major && minor == o.minor && patch == o.patch;
    }
    bool operator>=(const Version& o) const { return !(*this < o); }
    bool operator>(const Version& o) const { return o < *this; }
    bool operator<=(const Version& o) const { return !(o < *this); }

    QString toString() const {
        return QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
    }

    static Version fromString(const QString& s) {
        Version v;
        auto parts = s.split('.');
        if (parts.size() >= 1) v.major = parts[0].toInt();
        if (parts.size() >= 2) v.minor = parts[1].toInt();
        if (parts.size() >= 3) v.patch = parts[2].toInt();
        return v;
    }
};


struct LibraryInfo {
    QString name;       
    QString url;        
    QString sha1;       
    qint64 size = 0;    
    QString path;       
    bool isNative = false;
    QString nativeClassifier;
};


struct AssetInfo {
    QString name;
    QString hash;
    qint64 size = 0;
};

} 
