#pragma once

#include "TokenStore.h"

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

namespace x4::auth {

class LinuxTokenStore : public TokenStore {
public:
    LinuxTokenStore();
    ~LinuxTokenStore() override;

    bool saveToken(const QString& key, const QString& token) override;
    QString loadToken(const QString& key) override;
    bool deleteToken(const QString& key) override;
};

} // namespace x4::auth

#endif
