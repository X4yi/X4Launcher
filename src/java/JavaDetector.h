#pragma once

#include "JavaVersion.h"
#include <QObject>
#include <QVector>

namespace x4 {
    class JavaDetector : public QObject {
        Q_OBJECT

    public:
        explicit JavaDetector(QObject *parent = nullptr);


        void detectAll();


        static JavaVersion probeJavaBinary(const QString &javaPath);

        const QVector<JavaVersion> &results() const { return results_; }

        signals:
    

        void detected(const QVector<JavaVersion> &javas);

    private:
        QStringList searchPaths() const;

        QVector<JavaVersion> results_;
    };
}