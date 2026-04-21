#pragma once

#include "../core/Types.h"
#include <QObject>
#include <QString>

namespace x4 {

class DownloadManager;



class LibraryCache : public QObject {
    Q_OBJECT
public:
    explicit LibraryCache(QObject* parent = nullptr);

    
    QString getPath(const LibraryInfo& lib) const;

    
    bool has(const LibraryInfo& lib) const;

    
    
    bool ensureCached(const LibraryInfo& lib, DownloadManager& dm);

    
    bool verify(const LibraryInfo& lib) const;

private:
    QString storagePath(const LibraryInfo& lib) const;
};

} 
