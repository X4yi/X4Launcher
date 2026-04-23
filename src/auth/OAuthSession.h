#pragma once

#include "PkceGenerator.h"
#include <QString>
#include <QUuid>

namespace x4::auth {

class OAuthSession {
public:
    OAuthSession();

    QString state() const;
    QString pkceVerifier() const;
    QString pkceChallenge() const;
    QString pkceChallengeMethod() const;

private:
    QString state_;
    PkceGenerator pkce_;
};

} // namespace x4::auth
