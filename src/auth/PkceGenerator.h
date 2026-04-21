#pragma once

#include <QString>

namespace x4::auth {

class PkceGenerator {
public:
    static PkceGenerator generate();

    QString verifier() const;
    QString challenge() const;
    QString challengeMethod() const;

private:
    PkceGenerator(const QString& verifier, const QString& challenge);

    QString verifier_;
    QString challenge_;
};

} // namespace x4::auth
