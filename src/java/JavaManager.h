#pragma once

#include "JavaVersion.h"
#include "JavaDetector.h"
#include "JavaProvisioner.h"
#include <QObject>
#include <QVector>

namespace x4 {
    class JavaManager : public QObject {
        Q_OBJECT

    public:
        explicit JavaManager(QObject *parent = nullptr);


        void init();


        JavaVersion findJavaForMC(const QString &mcVersion) const;


        static int requiredJavaMajor(const QString &mcVersion);


        const QVector<JavaVersion> &detectedJavas() const { return detector_.results(); }


        void provisionJava(int majorVersion);


        void refresh();

        signals:
    

        void detectionFinished();

        void provisionProgress(int percent, const QString &status);

        void provisionFinished(bool success, const QString &error);

    private:
        JavaDetector detector_;
        JavaProvisioner provisioner_;
    };
}