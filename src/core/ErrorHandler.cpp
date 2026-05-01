#include "ErrorHandler.h"
#include "Translator.h"
#include <QRegularExpression>

namespace x4 {
    ErrorHandler &ErrorHandler::instance() {
        static ErrorHandler eh;
        return eh;
    }

    void ErrorHandler::initDefaults() {
        // Java Errors
        registerPattern(QStringLiteral("(?i)No suitable Java found"),
                        ErrorCategory::Java,
                        QStringLiteral("err.cat.java"),
                        QStringLiteral("No compatible Java installation was found for this Minecraft version."),
                        {
                            QStringLiteral("Install the correct Java version from Settings"),
                            QStringLiteral("Ensure Java path is correct")
                        });

        registerPattern(QStringLiteral("(?i)Java path is invalid"),
                        ErrorCategory::Java,
                        QStringLiteral("err.cat.java"),
                        QStringLiteral("The configured Java executable path does not exist or is not valid."),
                        {
                            QStringLiteral("Check Java settings"),
                            QStringLiteral("Use Auto-detect Java")
                        });

        registerPattern(QStringLiteral("(?i)Xmx < Xms"),
                        ErrorCategory::Java,
                        QStringLiteral("err.cat.java"),
                        QStringLiteral("Minimum memory (-Xms) is set higher than maximum memory (-Xmx)."),
                        {QStringLiteral("Fix memory allocation in Settings > Minecraft")});

        // Network Errors
        registerPattern(QStringLiteral("(?i)Failed to download version JSON"),
                        ErrorCategory::Network,
                        QStringLiteral("err.cat.net"),
                        QStringLiteral("Could not fetch the version metadata from Mojang servers."),
                        {
                            QStringLiteral("Check your internet connection"),
                            QStringLiteral("Try again later")
                        });

        // Mod Loader Errors
        registerPattern(QStringLiteral("(?i)No compatible.*version found"),
                        ErrorCategory::ModLoader,
                        QStringLiteral("err.cat.mod"),
                        QStringLiteral("No matching mod loader version for this Minecraft release."),
                        {
                            QStringLiteral("Select a different Minecraft version"),
                            QStringLiteral("Use a different mod loader (e.g., Fabric instead of Forge)")
                        });

        // Launch Errors
        registerPattern(QStringLiteral("(?i)OutOfMemoryError"),
                        ErrorCategory::Launch,
                        QStringLiteral("err.cat.launch"),
                        QStringLiteral("Minecraft ran out of allocated memory (RAM)."),
                        {
                            QStringLiteral("Increase Max Memory (-Xmx) in Settings"),
                            QStringLiteral("Remove memory-heavy mods or resource packs")
                        });

        registerPattern(QStringLiteral("(?i)DISPLAY.*unavailable"),
                        ErrorCategory::Launch,
                        QStringLiteral("err.cat.launch"),
                        QStringLiteral("The X11 Display Server or XWayland could not be reached."),
                        {
                            QStringLiteral("Install xorg-xwayland if using Wayland"),
                            QStringLiteral("Run inside a standard desktop session")
                        });

        registerPattern(QStringLiteral("(?i)GAME_CRASH"),
                        ErrorCategory::Launch,
                        QStringLiteral("err.cat.launch"),
                        QStringLiteral("The game process exited unexpectedly."),
                        {
                            QStringLiteral("Check the generated crash report"),
                            QStringLiteral("Remove recently added mods"),
                            QStringLiteral("Verify game files")
                        });

        // FileSystem
        registerPattern(QStringLiteral("(?i)Missing classpath entry"),
                        ErrorCategory::FileSystem,
                        QStringLiteral("err.cat.fs"),
                        QStringLiteral("A required library file is missing from the disk."),
                        {
                            QStringLiteral("Launch again to re-download libraries"),
                            QStringLiteral("Clear launcher cache")
                        });

        registerPattern(QStringLiteral("(?i)client JAR is missing"),
                        ErrorCategory::FileSystem,
                        QStringLiteral("err.cat.fs"),
                        QStringLiteral("The main Minecraft game file is missing."),
                        {
                            QStringLiteral("Check network connection"),
                            QStringLiteral("Ensure you have disk space")
                        });
    }

    void ErrorHandler::registerPattern(const QString &pattern, ErrorCategory cat,
                                       const QString &title, const QString &cause,
                                       const QStringList &actions) {
        patterns_.append({pattern, cat, title, cause, actions});
    }

    ErrorDiagnosis ErrorHandler::diagnose(const QString &errorMessage) const {
        for (const auto &pattern: patterns_) {
            QRegularExpression re(pattern.regex);
            if (re.match(errorMessage).hasMatch()) {
                return x4::ErrorDiagnosis{
                    pattern.category,
                    x4::Translator::instance().get(pattern.title),
                    pattern.cause,
                    pattern.actions,
                    errorMessage
                };
            }
        }

        // Default unknown error
        return x4::ErrorDiagnosis{
            ErrorCategory::Unknown,
            x4::Translator::instance().get(QStringLiteral("err.cat.unknown")),
            QStringLiteral("An unexpected error occurred during execution."),
            {
                QStringLiteral("Check launcher logs"),
                QStringLiteral("Report this issue on GitHub")
            },
            errorMessage
        };
    }
} // namespace x4