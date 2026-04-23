#pragma once

#include "TokenStore.h"

#ifdef Q_OS_WIN

namespace x4::auth {

class WindowsTokenStore : public TokenStore {
public:
    WindowsTokenStore();
    ~WindowsTokenStore() override;

    bool saveToken(const QString& key, const QString& token) override;
    QString loadToken(const QString& key) override;
    bool deleteToken(const QString& key) override;

private:
    QString getTargetName(const QString& key) const;
};

} // namespace x4::auth

#endif
