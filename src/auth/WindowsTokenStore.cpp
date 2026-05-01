#include "WindowsTokenStore.h"

#ifdef Q_OS_WIN

#include <windows.h>
#include <wincred.h>

namespace x4::auth {
    WindowsTokenStore::WindowsTokenStore() = default;

    WindowsTokenStore::~WindowsTokenStore() = default;

    QString WindowsTokenStore::getTargetName(const QString &key) const {
        return QStringLiteral("X4Launcher_") + key;
    }

    bool WindowsTokenStore::saveToken(const QString &key, const QString &token) {
        QString targetName = getTargetName(key);
        QByteArray tokenData = token.toUtf8();

        CREDENTIALW cred = {0};
        cred.Type = CRED_TYPE_GENERIC;
        cred.TargetName = (LPWSTR) targetName.utf16();
        cred.CredentialBlobSize = tokenData.size();
        cred.CredentialBlob = (LPBYTE) tokenData.constData();
        cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

        return CredWriteW(&cred, 0);
    }

    QString WindowsTokenStore::loadToken(const QString &key) {
        QString targetName = getTargetName(key);
        PCREDENTIALW cred = nullptr;

        if (CredReadW((LPCWSTR) targetName.utf16(), CRED_TYPE_GENERIC, 0, &cred)) {
            QString token = QString::fromUtf8((const char *) cred->CredentialBlob, cred->CredentialBlobSize);
            CredFree(cred);
            return token;
        }

        return QString();
    }

    bool WindowsTokenStore::deleteToken(const QString &key) {
        QString targetName = getTargetName(key);
        return CredDeleteW((LPCWSTR) targetName.utf16(), CRED_TYPE_GENERIC, 0);
    }
} // namespace x4::auth

#endif