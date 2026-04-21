// GLib/libsecret headers MUST come before Qt headers to avoid
// the `signals` macro conflict with GDBusInterfaceInfo.
#include <libsecret/secret.h>
#include <glib.h>

#include "LinuxTokenStore.h"

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

namespace x4::auth {

namespace {

const SecretSchema* getSecretSchema() {
    static SecretSchema schema;
    static bool initialized = false;
    if (!initialized) {
        schema.name = "org.x4launcher.Auth";
        schema.flags = SECRET_SCHEMA_NONE;
        schema.attributes[0] = {"account", SECRET_SCHEMA_ATTRIBUTE_STRING};
        schema.attributes[1] = {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING};
        initialized = true;
    }
    return &schema;
}

}

LinuxTokenStore::LinuxTokenStore() = default;

LinuxTokenStore::~LinuxTokenStore() = default;

bool LinuxTokenStore::saveToken(const QString& key, const QString& token) {
    GError* error = nullptr;
    QByteArray keyData = key.toUtf8();
    QByteArray tokenData = token.toUtf8();

    gboolean result = secret_password_store_sync(
        getSecretSchema(),
        SECRET_COLLECTION_DEFAULT,
        "X4Launcher Auth Token",
        tokenData.constData(),
        nullptr,
        &error,
        "account", keyData.constData(),
        nullptr
    );

    if (error != nullptr) {
        g_error_free(error);
        return false;
    }

    return result;
}

QString LinuxTokenStore::loadToken(const QString& key) {
    GError* error = nullptr;
    QByteArray keyData = key.toUtf8();

    gchar* password = secret_password_lookup_sync(
        getSecretSchema(),
        nullptr,
        &error,
        "account", keyData.constData(),
        nullptr
    );

    if (error != nullptr) {
        g_error_free(error);
        return QString();
    }

    if (password != nullptr) {
        QString token = QString::fromUtf8(password);
        secret_password_free(password);
        return token;
    }

    return QString();
}

bool LinuxTokenStore::deleteToken(const QString& key) {
    GError* error = nullptr;
    QByteArray keyData = key.toUtf8();

    gboolean result = secret_password_clear_sync(
        getSecretSchema(),
        nullptr,
        &error,
        "account", keyData.constData(),
        nullptr
    );

    if (error != nullptr) {
        g_error_free(error);
        return false;
    }

    return result;
}

} // namespace x4::auth

#endif
