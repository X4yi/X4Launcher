#include "KeyBindManager.h"
#include <QWidget>

namespace x4 {

KeyBindManager::KeyBindManager(QWidget* parent)
    : QObject(parent), parent_(parent)
{
    registerDefaults();
}

void KeyBindManager::registerDefaults() {
    registerBind(KeyBind::LaunchInstance,    QKeySequence(Qt::CTRL | Qt::Key_L),        QStringLiteral("Launch Instance"));
    registerBind(KeyBind::EditInstance,      QKeySequence(Qt::CTRL | Qt::Key_I),        QStringLiteral("Edit Instance"));
    registerBind(KeyBind::AddInstance,       QKeySequence(Qt::CTRL | Qt::Key_N),        QStringLiteral("Add Instance"));
    registerBind(KeyBind::DeleteInstance,    QKeySequence(Qt::Key_Delete),               QStringLiteral("Delete Instance"));
    registerBind(KeyBind::CloneInstance,     QKeySequence(Qt::CTRL | Qt::Key_D),        QStringLiteral("Clone Instance"));
    registerBind(KeyBind::OpenInstanceFolder, QKeySequence(Qt::CTRL | Qt::Key_O),       QStringLiteral("Open Folder"));
    registerBind(KeyBind::GlobalSettings,   QKeySequence(Qt::Key_F12),                  QStringLiteral("Global Settings"));
    registerBind(KeyBind::FocusSearch,      QKeySequence(Qt::CTRL | Qt::Key_F),         QStringLiteral("Search"));
    registerBind(KeyBind::Quit,             QKeySequence(Qt::CTRL | Qt::Key_Q),         QStringLiteral("Quit"));
}

QKeySequence KeyBindManager::keyFor(KeyBind bind) const {
    auto it = binds_.find(bind);
    return it != binds_.end() ? it->sequence : QKeySequence();
}

QString KeyBindManager::displayName(KeyBind bind) const {
    auto it = binds_.find(bind);
    return it != binds_.end() ? it->name : QString();
}

void KeyBindManager::registerBind(KeyBind bind, const QKeySequence& seq, const QString& name) {
    auto* action = new QAction(name, parent_);
    action->setShortcut(seq);
    action->setShortcutContext(Qt::WindowShortcut);
    parent_->addAction(action);

    switch (bind) {
    case KeyBind::LaunchInstance:    connect(action, &QAction::triggered, this, &KeyBindManager::launchInstanceTriggered); break;
    case KeyBind::EditInstance:      connect(action, &QAction::triggered, this, &KeyBindManager::editInstanceTriggered); break;
    case KeyBind::AddInstance:       connect(action, &QAction::triggered, this, &KeyBindManager::addInstanceTriggered); break;
    case KeyBind::DeleteInstance:    connect(action, &QAction::triggered, this, &KeyBindManager::deleteInstanceTriggered); break;
    case KeyBind::CloneInstance:     connect(action, &QAction::triggered, this, &KeyBindManager::cloneInstanceTriggered); break;
    case KeyBind::OpenInstanceFolder: connect(action, &QAction::triggered, this, &KeyBindManager::openInstanceFolderTriggered); break;
    case KeyBind::GlobalSettings:   connect(action, &QAction::triggered, this, &KeyBindManager::globalSettingsTriggered); break;
    case KeyBind::FocusSearch:      connect(action, &QAction::triggered, this, &KeyBindManager::focusSearchTriggered); break;
    case KeyBind::Quit:             connect(action, &QAction::triggered, this, &KeyBindManager::quitTriggered); break;
    }

    binds_[bind] = {seq, name, action};
}

} 
