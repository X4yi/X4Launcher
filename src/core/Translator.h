#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

namespace x4 {
    class Translator {
    public:
        static Translator &instance();

        void setLanguage(const QString &lang);

        QString language() const { return currentLang_; }

        QStringList availableLanguages() const;

        QString languageDisplayName(const QString &lang) const;

        // Lookup with positional argument substitution (%1, %2, %3)
        QString get(const QString &key) const;

        QString get(const QString &key, const QString &a1) const;

        QString get(const QString &key, const QString &a1, const QString &a2) const;

        QString get(const QString &key, int a1) const;

        QString get(const QString &key, int a1, int a2) const;

        QString get(const QString &key, const QString &a1, int a2) const;

    private:
        Translator();

        void loadEnglish();

        void loadSpanish();

        QHash<QString, QHash<QString, QString> > translations_; // lang -> {key -> value}
        QString currentLang_ = QStringLiteral("en");
    };

    // Convenience shorthand
#define X4TR(key) ::x4::Translator::instance().get(QStringLiteral(key))
#define X4TR1(key, a1) ::x4::Translator::instance().get(QStringLiteral(key), a1)
#define X4TR2(key, a1, a2) ::x4::Translator::instance().get(QStringLiteral(key), a1, a2)
} // namespace x4