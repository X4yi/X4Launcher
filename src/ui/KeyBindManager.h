#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <QAction>

class QWidget;

namespace x4 {
    enum class KeyBind {
        LaunchInstance,
        EditInstance,
        AddInstance,
        DeleteInstance,
        CloneInstance,
        OpenInstanceFolder,
        GlobalSettings,
        FocusSearch,
        Quit
    };

    class KeyBindManager : public QObject {
        Q_OBJECT

    public:
        explicit KeyBindManager(QWidget *parent);

        void registerDefaults();

        QKeySequence keyFor(KeyBind bind) const;

        QString displayName(KeyBind bind) const;

        signals:
    

        void launchInstanceTriggered();

        void editInstanceTriggered();

        void addInstanceTriggered();

        void deleteInstanceTriggered();

        void cloneInstanceTriggered();

        void openInstanceFolderTriggered();

        void globalSettingsTriggered();

        void focusSearchTriggered();

        void quitTriggered();

    private:
        void registerBind(KeyBind bind, const QKeySequence &seq, const QString &name);

        struct BindInfo {
            QKeySequence sequence;
            QString name;
            QAction *action = nullptr;
        };

        QWidget *parent_;
        QMap<KeyBind, BindInfo> binds_;
    };
}