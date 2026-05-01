#include "JavaProvisioner.h"
#include "JavaDetector.h"
#include "../core/PathManager.h"
#include "../core/Logger.h"
#include "../network/HttpClient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QtConcurrent>

namespace x4 {
    JavaProvisioner::JavaProvisioner(QObject *parent) : QObject(parent) {
    }

    QString JavaProvisioner::osName() {
#ifdef Q_OS_WIN
        return QStringLiteral("windows");
#elif defined(Q_OS_LINUX)
        return QStringLiteral("linux");
#elif defined(Q_OS_MACOS)
        return QStringLiteral("mac");
#else
        return QStringLiteral("linux");
#endif
    }

    QString JavaProvisioner::archName() {
#if defined(Q_PROCESSOR_X86_64)
        return QStringLiteral("x64");
#elif defined(Q_PROCESSOR_ARM_64)
        return QStringLiteral("aarch64");
#else
        return QStringLiteral("x64");
#endif
    }

    void JavaProvisioner::provision(int majorVersion) {
        emit progress(0, QStringLiteral("Fetching Java %1 metadata...").arg(majorVersion));

        QString url = QStringLiteral(
                    "https://api.adoptium.net/v3/assets/latest/%1/hotspot?"
                    "architecture=%2&image_type=jdk&os=%3&vendor=eclipse")
                .arg(majorVersion).arg(archName(), osName());

        auto *reply = HttpClient::instance().get(QUrl(url));
        connect(reply, &QNetworkReply::finished, this, [this, reply, majorVersion]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                emit finished(false, {}, QStringLiteral("Failed to fetch Java metadata: %1").arg(reply->errorString()));
                return;
            }
            onMetadataReceived(reply->readAll(), majorVersion);
        });
    }

    void JavaProvisioner::onMetadataReceived(const QByteArray &data, int majorVersion) {
        auto doc = QJsonDocument::fromJson(data);
        if (!doc.isArray() || doc.array().isEmpty()) {
            emit finished(false, {}, QStringLiteral("No Java %1 releases found for this platform").arg(majorVersion));
            return;
        }

        auto obj = doc.array().first().toObject();
        auto binary = obj[QStringLiteral("binary")].toObject();
        auto pkg = binary[QStringLiteral("package")].toObject();

        QString downloadUrl = pkg[QStringLiteral("link")].toString();
        QString checksum = pkg[QStringLiteral("checksum")].toString();
        QString name = pkg[QStringLiteral("name")].toString();

        if (downloadUrl.isEmpty()) {
            emit finished(false, {}, QStringLiteral("No download URL in Adoptium response"));
            return;
        }

        emit progress(10, QStringLiteral("Downloading Java %1...").arg(majorVersion));

        QString destDir = PathManager::instance().javaDir();
        QString archivePath = destDir + QStringLiteral("/") + name;

        auto *reply = HttpClient::instance().get(QUrl(downloadUrl));

        connect(reply, &QNetworkReply::downloadProgress, this,
                [this, majorVersion](qint64 recv, qint64 total) {
                    if (total > 0) {
                        int pct = 10 + static_cast<int>(80.0 * recv / total);
                        emit progress(pct, QStringLiteral("Downloading Java %1... %2%")
                                      .arg(majorVersion).arg(pct));
                    }
                });

        connect(reply, &QNetworkReply::finished, this,
                [this, reply, archivePath, destDir, majorVersion]() {
                    reply->deleteLater();
                    if (reply->error() != QNetworkReply::NoError) {
                        emit finished(false, {}, QStringLiteral("Download failed: %1").arg(reply->errorString()));
                        return;
                    }


                    QFile file(archivePath);
                    if (!file.open(QIODevice::WriteOnly)) {
                        emit finished(false, {}, QStringLiteral("Cannot write archive: %1").arg(file.errorString()));
                        return;
                    }
                    file.write(reply->readAll());
                    file.close();

                    extractArchive(archivePath, destDir, majorVersion);
                });
    }

    void JavaProvisioner::extractArchive(const QString &archivePath, const QString &destDir, int majorVersion) {
        emit progress(90, QStringLiteral("Extracting Java %1...").arg(majorVersion));


        auto future = QtConcurrent::run([archivePath, destDir]() -> bool {
            QProcess proc;
            proc.setWorkingDirectory(destDir);

#ifdef Q_OS_WIN

            proc.start(QStringLiteral("tar"), {QStringLiteral("-xf"), archivePath});
#else
            if (archivePath.endsWith(QStringLiteral(".tar.gz")) ||
                archivePath.endsWith(QStringLiteral(".tgz"))) {
                proc.start(QStringLiteral("tar"), {QStringLiteral("-xzf"), archivePath});
            } else if (archivePath.endsWith(QStringLiteral(".zip"))) {
                proc.start(QStringLiteral("unzip"), {QStringLiteral("-o"), archivePath, QStringLiteral("-d"), destDir});
            } else {
                proc.start(QStringLiteral("tar"), {QStringLiteral("-xf"), archivePath});
            }
#endif
            if (!proc.waitForFinished(120000)) return false;
            bool ok = proc.exitCode() == 0;


            QFile::remove(archivePath);
            return ok;
        });

        auto *watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this,
                [this, watcher, destDir, majorVersion]() {
                    watcher->deleteLater();
                    if (!watcher->result()) {
                        emit finished(false, {}, QStringLiteral("Failed to extract Java archive"));
                        return;
                    }

                    emit progress(95, QStringLiteral("Verifying Java %1...").arg(majorVersion));


                    QDirIterator it(destDir, QDir::Dirs | QDir::NoDotAndDotDot);
                    while (it.hasNext()) {
                        it.next();
#ifdef Q_OS_WIN
                        QString javaExe = it.filePath() + QStringLiteral("/bin/java.exe");
#else
                        QString javaExe = it.filePath() + QStringLiteral("/bin/java");
#endif
                        if (QFileInfo::exists(javaExe)) {
                            auto ver = JavaDetector::probeJavaBinary(javaExe);
                            if (ver.isValid() && ver.major == majorVersion) {
                                ver.isManaged = true;
                                emit progress(100, QStringLiteral("Java %1 installed").arg(majorVersion));
                                emit finished(true, ver, {});
                                return;
                            }
                        }
                    }

                    emit finished(false, {}, QStringLiteral("Could not find java binary after extraction"));
                });
        watcher->setFuture(future);
    }
}