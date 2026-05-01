#include "OAuthSession.h"

namespace x4::auth {
    OAuthSession::OAuthSession()
        : state_(QUuid::createUuid().toString(QUuid::WithoutBraces)),
          pkce_(PkceGenerator::generate()) {
    }

    QString OAuthSession::state() const {
        return state_;
    }

    QString OAuthSession::pkceVerifier() const {
        return pkce_.verifier();
    }

    QString OAuthSession::pkceChallenge() const {
        return pkce_.challenge();
    }

    QString OAuthSession::pkceChallengeMethod() const {
        return pkce_.challengeMethod();
    }
} // namespace x4::auth