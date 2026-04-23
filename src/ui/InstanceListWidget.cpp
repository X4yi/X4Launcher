#include "InstanceListWidget.h"
#include "InstanceDelegate.h"
#include "../core/SettingsManager.h"
#include "../instance/InstanceManager.h"
#include <QMenu>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>

namespace x4 {

InstanceListWidget::InstanceListWidget(InstanceManager* mgr, QWidget* parent)
    : QListView(parent)
    , mgr_(mgr)
    , zenMode_(false)
{
    model_ = new QStandardItemModel(this);
    delegate_ = new InstanceDelegate(this);

    setModel(model_);
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(false);
    setMouseTracking(true);

    setZenMode(SettingsManager::instance().zenMode());

    
    connect(mgr_, &InstanceManager::instanceAdded, this, [this](const Instance& inst) {
        addInstanceItem(inst);
    });
    connect(mgr_, &InstanceManager::instanceRemoved, this, [this](const QString& id) {
        refresh();
    });
    connect(mgr_, &InstanceManager::instanceChanged, this, [this](const Instance& inst) {
        
        for (int i = 0; i < model_->rowCount(); i++) {
            auto* item = model_->item(i);
            if (item->data(InstanceDelegate::IdRole).toString() == inst.id) {
                item->setData(inst.name, InstanceDelegate::NameRole);
                item->setData(inst.mcVersion, InstanceDelegate::VersionRole);
                item->setData(modLoaderName(inst.loader), InstanceDelegate::LoaderRole);
                item->setData(static_cast<int>(inst.state), InstanceDelegate::StateRole);
                item->setData(inst.group, InstanceDelegate::GroupRole);
                item->setData(inst.iconKey, InstanceDelegate::IconKeyRole);
                if (zenMode_) {
                    item->setText(inst.name + " [" + inst.mcVersion + "] [" + modLoaderName(inst.loader) + "]");
                }
                break;
            }
        }
        applyFilter();
        viewport()->update();
    });
}

void InstanceListWidget::refresh() {
    model_->clear();
    for (const auto& inst : mgr_->instances()) {
        addInstanceItem(inst);
    }
    applyFilter();
}

void InstanceListWidget::setZenMode(bool zen) {
    if (zenMode_ == zen) return;
    zenMode_ = zen;
    if (zenMode_) {
        setViewMode(QListView::ListMode);
        setFlow(QListView::TopToBottom);
        setWrapping(false);
        setResizeMode(QListView::Fixed);
        setSpacing(2);
        setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        setItemDelegate(nullptr);  // Use default delegate for minimal resource usage
    } else {
        setViewMode(QListView::IconMode);
        setFlow(QListView::LeftToRight);
        setWrapping(true);
        setResizeMode(QListView::Adjust);
        setSpacing(8);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setItemDelegate(delegate_);  // Use custom delegate for icons
    }

    for (int i = 0; i < model_->rowCount(); i++) {
        auto* item = model_->item(i);
        if (!item) continue;

        if (zenMode_) {
            QString name = item->data(InstanceDelegate::NameRole).toString();
            QString version = item->data(InstanceDelegate::VersionRole).toString();
            QString loader = item->data(InstanceDelegate::LoaderRole).toString();
            item->setText(name + " [" + version + "] [" + loader + "]");
        } else {
            item->setText(QString());
        }
    }

    refresh();  // Refresh to update content display
}

void InstanceListWidget::setGroupFilter(const QString& group) {
    currentGroupFilter_ = group;
    applyFilter();
}

void InstanceListWidget::applyFilter() {
    for (int i = 0; i < model_->rowCount(); i++) {
        auto* item = model_->item(i);
        QString group = item->data(InstanceDelegate::GroupRole).toString();
        bool show = currentGroupFilter_.isEmpty() || group == currentGroupFilter_;
        setRowHidden(i, !show);
    }
}

QStringList InstanceListWidget::availableGroups() const {
    QStringList groups;
    for (const auto& inst : mgr_->instances()) {
        if (!inst.group.isEmpty() && !groups.contains(inst.group)) {
            groups.append(inst.group);
        }
    }
    groups.sort();
    return groups;
}

void InstanceListWidget::addInstanceItem(const Instance& inst) {
    auto* item = new QStandardItem();
    item->setData(inst.id, InstanceDelegate::IdRole);
    item->setData(inst.name, InstanceDelegate::NameRole);
    item->setData(inst.mcVersion, InstanceDelegate::VersionRole);
    item->setData(modLoaderName(inst.loader), InstanceDelegate::LoaderRole);
    item->setData(static_cast<int>(inst.state), InstanceDelegate::StateRole);
    item->setData(inst.group, InstanceDelegate::GroupRole);
    item->setData(inst.iconKey, InstanceDelegate::IconKeyRole);
    item->setEditable(false);
    if (zenMode_) {
        item->setText(inst.name + " [" + inst.mcVersion + "] [" + modLoaderName(inst.loader) + "]");
    }
    model_->appendRow(item);
}

QString InstanceListWidget::selectedInstanceId() const {
    auto idx = currentIndex();
    if (!idx.isValid()) return {};
    return idx.data(InstanceDelegate::IdRole).toString();
}

void InstanceListWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    auto idx = indexAt(event->pos());
    if (idx.isValid()) {
        emit launchRequested(idx.data(InstanceDelegate::IdRole).toString());
    }
    QListView::mouseDoubleClickEvent(event);
}

void InstanceListWidget::contextMenuEvent(QContextMenuEvent* event) {
    auto idx = indexAt(event->pos());
    if (!idx.isValid()) return;

    QString id = idx.data(InstanceDelegate::IdRole).toString();
    QString name = idx.data(InstanceDelegate::NameRole).toString();

    auto* menu = new QMenu(this);

    menu->addAction(QStringLiteral("▶  Launch"), [this, id]() {
        emit launchRequested(id);
    });

    menu->addSeparator();

    menu->addAction(QStringLiteral("✎  Edit"), [this, id]() {
        emit editRequested(id);
    });

    menu->addAction(QStringLiteral("🏷  Change Group"), [this, id]() {
        bool ok;
        QString currentGroup;
        if (auto* inst = mgr_->findById(id)) {
            currentGroup = inst->group;
        }
        QString newGroup = QInputDialog::getText(this, QStringLiteral("Change Group"),
            QStringLiteral("Group name:"), QLineEdit::Normal, currentGroup, &ok);
        if (ok) {
            mgr_->setInstanceGroup(id, newGroup.trimmed());
        }
    });

    menu->addAction(QStringLiteral("⎘  Clone"), [this, id]() {
        emit cloneRequested(id);
    });

    menu->addAction(QStringLiteral("📂  Open Folder"), [this, id]() {
        QString dir = mgr_->instanceDir(id);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });

    menu->addSeparator();

    auto* deleteAction = menu->addAction(QStringLiteral("🗑  Delete"), [this, id]() {
        emit deleteRequested(id);
    });
    deleteAction->setObjectName(QStringLiteral("deleteAction"));

    menu->exec(event->globalPos());
    menu->deleteLater();
}

} 
