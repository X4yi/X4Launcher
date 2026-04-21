#include "ModrinthAPI.h"
#include <QNetworkRequest>
#include <QJsonParseError>
#include <QDebug>
#include "../network/HttpClient.h"
namespace x4 {

ModrinthAPI::ModrinthAPI(QObject* parent) : QObject(parent) {
    networkManager_ = HttpClient::instance().manager();
}

ModrinthAPI::~ModrinthAPI() = default;

QString ModrinthAPI::buildFacets(const QString& mcVersion, ModLoaderType loader, const QString& projectType) {
    QJsonArray outerArray;
    
    if (!mcVersion.isEmpty()) {
        QJsonArray mc;
        mc.append(QString("versions:%1").arg(mcVersion));
        outerArray.append(mc);
    }

    if (loader != ModLoaderType::Vanilla && projectType == "mod") {
        QJsonArray load;
        load.append(QString("categories:%1").arg(modLoaderName(loader).toLower()));
        outerArray.append(load);
    }

    if (!projectType.isEmpty()) {
        QJsonArray pt;
        pt.append(QString("project_type:%1").arg(projectType));
        outerArray.append(pt);
    }

    QJsonDocument doc(outerArray);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void ModrinthAPI::setupRequest(QNetworkRequest& req) {
    req.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT);
    req.setRawHeader("Accept", "application/json");
}

void ModrinthAPI::searchProjects(const QString& query, const QString& facets, int limit, int offset, const QString& index) {
    QUrl url(API_URL + "/search");
    QUrlQuery q;
    q.addQueryItem("query", query);
    q.addQueryItem("limit", QString::number(limit));
    q.addQueryItem("offset", QString::number(offset));
    q.addQueryItem("index", index);
    if (!facets.isEmpty()) {
        q.addQueryItem("facets", facets);
    }
    url.setQuery(q);

    QNetworkRequest req(url);
    setupRequest(req);

    auto* reply = networkManager_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit searchFailed(reply->errorString());
            return;
        }

        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            emit searchFailed("Invalid JSON response");
            return;
        }

        auto root = doc.object();
        ModrinthSearchResult result;
        result.totalHits = root["total_hits"].toInt();
        result.offset = root["offset"].toInt();
        result.limit = root["limit"].toInt();

        auto hitsArray = root["hits"].toArray();
        for (const auto& v : hitsArray) {
            result.hits.append(parseProject(v.toObject()));
        }

        emit searchFinished(result);
    });
}

void ModrinthAPI::getProjectVersions(const QString& projectId, const QString& mcVersion, const QString& loader) {
    QUrl url(API_URL + "/project/" + projectId + "/version");
    QUrlQuery q;
    if (!mcVersion.isEmpty()) {
        QJsonArray vers;
        vers.append(mcVersion);
        q.addQueryItem("game_versions", QString::fromUtf8(QJsonDocument(vers).toJson(QJsonDocument::Compact)));
    }
    if (!loader.isEmpty()) {
        QJsonArray loads;
        loads.append(loader);
        q.addQueryItem("loaders", QString::fromUtf8(QJsonDocument(loads).toJson(QJsonDocument::Compact)));
    }
    url.setQuery(q);

    QNetworkRequest req(url);
    setupRequest(req);

    auto* reply = networkManager_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, projectId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit versionsFailed(projectId, reply->errorString());
            return;
        }

        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isArray()) {
            emit versionsFailed(projectId, "Invalid JSON response");
            return;
        }

        QVector<ModrinthVersion> versions;
        auto arr = doc.array();
        for (const auto& v : arr) {
            versions.append(parseVersion(v.toObject()));
        }

        emit versionsFinished(projectId, versions);
    });
}

void ModrinthAPI::getVersion(const QString& versionId) {
    QUrl url(API_URL + "/version/" + versionId);
    QNetworkRequest req(url);
    setupRequest(req);

    auto* reply = networkManager_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, versionId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit versionFailed(versionId, reply->errorString());
            return;
        }

        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            emit versionFailed(versionId, "Invalid JSON response");
            return;
        }

        emit versionFinished(parseVersion(doc.object()));
    });
}

ModrinthProject ModrinthAPI::parseProject(const QJsonObject& obj) {
    ModrinthProject p;
    p.id = obj["project_id"].toString(); 
    if (p.id.isEmpty()) p.id = obj["id"].toString(); 
    
    p.slug = obj["slug"].toString();
    p.title = obj["title"].toString();
    p.description = obj["description"].toString();
    p.iconUrl = obj["icon_url"].toString();
    p.author = obj["author"].toString();
    p.downloads = obj["downloads"].toInt();
    p.projectType = obj["project_type"].toString();

    auto cats = obj["categories"].toArray();
    for (const auto& c : cats) {
        p.categories.append(c.toString());
    }
    
    return p;
}

ModrinthVersion ModrinthAPI::parseVersion(const QJsonObject& obj) {
    ModrinthVersion v;
    v.id = obj["id"].toString();
    v.name = obj["name"].toString();
    v.versionNumber = obj["version_number"].toString();

    auto gvs = obj["game_versions"].toArray();
    for (const auto& g : gvs) v.gameVersions.append(g.toString());

    auto loads = obj["loaders"].toArray();
    for (const auto& l : loads) v.loaders.append(l.toString());

    auto filesArr = obj["files"].toArray();
    for (const auto& f : filesArr) {
        auto fileObj = f.toObject();
        ModrinthFile mf;
        mf.url = fileObj["url"].toString();
        mf.filename = fileObj["filename"].toString();
        mf.size = fileObj["size"].toInt();
        mf.primary = fileObj["primary"].toBool();
        
        auto hashes = fileObj["hashes"].toObject();
        mf.sha1 = hashes["sha1"].toString();
        
        v.files.append(mf);
    }
    
    auto deps = obj["dependencies"].toArray();
    for (const auto& d : deps) {
        auto dObj = d.toObject();
        ModrinthDependency md;
        md.versionId = dObj["version_id"].toString();
        md.projectId = dObj["project_id"].toString();
        md.dependencyType = dObj["dependency_type"].toString();
        if(!md.versionId.isEmpty() || !md.projectId.isEmpty()) {
            v.dependencies.append(md);
        }
    }
    
    return v;
}

void ModrinthAPI::batchGetByHashes(const QStringList& sha1List) {
    QUrl url(API_URL + "/version_files");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");

    QJsonObject payload;
    QJsonArray hashes;
    for (const auto& hash : sha1List) hashes.append(hash);
    payload["hashes"] = hashes;
    payload["algorithm"] = "sha1";

    auto* reply = networkManager_->post(req, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit batchHashesFailed(reply->errorString());
            return;
        }

        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            emit batchHashesFailed("Invalid JSON response from batch hashes");
            return;
        }

        auto root = doc.object();
        QMap<QString, ModrinthVersion> resultMap;
        for (auto it = root.begin(); it != root.end(); ++it) {
            resultMap.insert(it.key(), parseVersion(it.value().toObject()));
        }

        emit batchHashesFinished(resultMap);
    });
}

#include <functional>

class DependencyResolver : public QObject {
public:
    DependencyResolver(ModrinthAPI* api, const QString& mcVer, ModLoaderType ldr)
        : QObject(api), api_(api), mcVersion_(mcVer), loader_(ldr) {}

    void start(const QStringList& initialVersionIds, 
               std::function<void(QVector<ModrinthVersion>)> onDone,
               std::function<void(QString)> onFail) 
    {
        onFinished_ = std::move(onDone);
        onFailed_ = std::move(onFail);
        
        for (const auto& vid : initialVersionIds) {
            queueVersion(vid);
        }
        processNext();
    }

    QVector<ModrinthVersion> results;

private:
    std::function<void(QVector<ModrinthVersion>)> onFinished_;
    std::function<void(QString)> onFailed_;
    void queueVersion(const QString& id) {
        if (!visitedVersions_.contains(id) && !pendingVersions_.contains(id)) {
            pendingVersions_.append(id);
        }
    }

    void queueProject(const QString& id) {
        if (!visitedProjects_.contains(id) && !pendingProjects_.contains(id)) {
            pendingProjects_.append(id);
        }
    }

    void processNext() {
        if (!pendingVersions_.isEmpty()) {
            QString vid = pendingVersions_.takeFirst();
            visitedVersions_.insert(vid);

            auto* nm = HttpClient::instance().manager();
            
            QUrl url("https://api.modrinth.com/v2/version/" + vid);
            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::UserAgentHeader, "X4Launcher/0.1");
            req.setRawHeader("Accept", "application/json");
            
            auto* reply = nm->get(req);

            connect(reply, &QNetworkReply::finished, this, [=]() {
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError) {
                    if (onFailed_) onFailed_("Failed to fetch dependency version: " + vid);
                    return;
                }
                QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
                auto v = parseVersionData(obj);
                results.append(v);
                
                for (const auto& d : v.dependencies) {
                    if (d.dependencyType == "required") {
                        if (!d.versionId.isEmpty()) {
                            queueVersion(d.versionId);
                        } else if (!d.projectId.isEmpty()) {
                            queueProject(d.projectId);
                        }
                    }
                }
                processNext();
            });
            return;
        }

        if (!pendingProjects_.isEmpty()) {
            QString pid = pendingProjects_.takeFirst();
            visitedProjects_.insert(pid);

            QUrl url("https://api.modrinth.com/v2/project/" + pid + "/version");
            QUrlQuery q;
            if (!mcVersion_.isEmpty()) {
                QJsonArray vers; vers.append(mcVersion_);
                q.addQueryItem("game_versions", QString::fromUtf8(QJsonDocument(vers).toJson(QJsonDocument::Compact)));
            }
            if (loader_ != ModLoaderType::Vanilla) {
                QJsonArray loads; loads.append(modLoaderName(loader_).toLower());
                q.addQueryItem("loaders", QString::fromUtf8(QJsonDocument(loads).toJson(QJsonDocument::Compact)));
            }
            url.setQuery(q);

            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::UserAgentHeader, "X4Launcher/0.1");
            req.setRawHeader("Accept", "application/json");

            auto* nm = HttpClient::instance().manager();
            auto* reply = nm->get(req);

            connect(reply, &QNetworkReply::finished, this, [=]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::NoError) {
                    auto arr = QJsonDocument::fromJson(reply->readAll()).array();
                    if (!arr.isEmpty()) {
                        auto v = parseVersionData(arr.first().toObject());
                        results.append(v);
                        visitedVersions_.insert(v.id);
                        for (const auto& d : v.dependencies) {
                            if (d.dependencyType == "required") {
                                if (!d.versionId.isEmpty()) queueVersion(d.versionId);
                                else if (!d.projectId.isEmpty()) queueProject(d.projectId);
                            }
                        }
                    }
                }
                processNext();
            });
            return;
        }

        if (onFinished_) onFinished_(results);
    }

    ModrinthVersion parseVersionData(const QJsonObject& obj) {
        ModrinthVersion v;
        v.id = obj["id"].toString();
        v.name = obj["name"].toString();
        v.versionNumber = obj["version_number"].toString();
        
        auto filesArr = obj["files"].toArray();
        for (const auto& f : filesArr) {
            auto fileObj = f.toObject();
            ModrinthFile mf;
            mf.url = fileObj["url"].toString();
            mf.filename = fileObj["filename"].toString();
            mf.size = fileObj["size"].toInt();
            mf.primary = fileObj["primary"].toBool();
            mf.sha1 = fileObj["hashes"].toObject()["sha1"].toString();
            v.files.append(mf);
        }
        
        auto deps = obj["dependencies"].toArray();
        for (const auto& d : deps) {
            auto dObj = d.toObject();
            ModrinthDependency md;
            md.versionId = dObj["version_id"].toString();
            md.projectId = dObj["project_id"].toString();
            md.dependencyType = dObj["dependency_type"].toString();
            if(!md.versionId.isEmpty() || !md.projectId.isEmpty()) v.dependencies.append(md);
        }
        return v;
    }

    ModrinthAPI* api_;
    QString mcVersion_;
    ModLoaderType loader_;
    
    QSet<QString> visitedVersions_;
    QSet<QString> visitedProjects_;
    QStringList pendingVersions_;
    QStringList pendingProjects_;
};

void ModrinthAPI::resolveDependencies(const QStringList& versionIds, const QString& mcVersion, ModLoaderType loader) {
    auto* resolver = new DependencyResolver(this, mcVersion, loader);
    
    resolver->start(versionIds, [this, resolver](QVector<ModrinthVersion> res) {
        emit dependenciesResolved(res);
        resolver->deleteLater();
    }, [this, resolver](QString err) {
        emit dependenciesResolveFailed(err);
        resolver->deleteLater();
    });
}

}
