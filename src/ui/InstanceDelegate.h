#pragma once

#include <QStyledItemDelegate>
#include <QSet>

namespace x4 {
    class InstanceDelegate : public QStyledItemDelegate {
        Q_OBJECT

    public:
        explicit InstanceDelegate(QObject *parent = nullptr);

        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

        QSize sizeHint(const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override;


        void setRunning(const QString &instanceId, bool running);


        enum Roles {
            IdRole = Qt::UserRole + 1,
            NameRole,
            VersionRole,
            LoaderRole,
            StateRole,
            GroupRole,
            LastPlayedRole,
            IconKeyRole
        };

    private:
        QSet<QString> runningInstances_;
    };
}