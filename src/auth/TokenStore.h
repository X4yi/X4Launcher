#pragma once

#include <QString>

namespace x4::auth {

class TokenStore {
public:
    virtual ~TokenStore() = default;

    virtual bool saveToken(const QString& key, const QString& token) = 0;
    virtual QString loadToken(const QString& key) = 0;
    virtual bool deleteToken(const QString& key) = 0;
};

TokenStore* createTokenStore();

} // namespace x4::auth
