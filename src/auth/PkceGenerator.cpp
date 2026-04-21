#include "PkceGenerator.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QByteArray>

namespace x4::auth {

PkceGenerator::PkceGenerator(const QString& verifier, const QString& challenge)
    : verifier_(verifier), challenge_(challenge) {
}

PkceGenerator PkceGenerator::generate() {
    // Generate 32 bytes of random data for the verifier
    QByteArray randomBytes;
    randomBytes.resize(32);
    quint32* data = reinterpret_cast<quint32*>(randomBytes.data());
    for (int i = 0; i < 8; ++i) {
        data[i] = QRandomGenerator::global()->generate();
    }

    // Base64Url encode without padding
    QString verifier = QString::fromLatin1(randomBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));

    // Calculate S256 challenge: Base64UrlEncode(SHA256(ASCII(verifier)))
    QByteArray hash = QCryptographicHash::hash(verifier.toLatin1(), QCryptographicHash::Sha256);
    QString challenge = QString::fromLatin1(hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));

    return PkceGenerator(verifier, challenge);
}

QString PkceGenerator::verifier() const {
    return verifier_;
}

QString PkceGenerator::challenge() const {
    return challenge_;
}

QString PkceGenerator::challengeMethod() const {
    return QStringLiteral("S256");
}

} // namespace x4::auth
