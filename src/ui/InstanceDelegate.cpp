#include "InstanceDelegate.h"
#include "Theme.h"
#include "../core/SettingsManager.h"
#include <QPainter>
#include <QApplication>

namespace x4 {
    InstanceDelegate::InstanceDelegate(QObject *parent)
        : QStyledItemDelegate(parent) {
    }

    void InstanceDelegate::setRunning(const QString &instanceId, bool running) {
        if (running)
            runningInstances_.insert(instanceId);
        else
            runningInstances_.remove(instanceId);
    }

    QSize InstanceDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const {
        if (SettingsManager::instance().zenMode()) {
            return QSize(250, 24);
        }
        return QSize(110, 110);
    }

    void InstanceDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const {
        painter->save();
        if (!SettingsManager::instance().zenMode()) {
            painter->setRenderHint(QPainter::Antialiasing);
        }

        QRect rect = option.rect.adjusted(4, 4, -4, -4);


        QColor bgColor;
        if (option.state & QStyle::State_Selected) {
            bgColor = QColor(0x28, 0x34, 0x57);
        } else if (option.state & QStyle::State_MouseOver) {
            bgColor = QColor(0x1f, 0x23, 0x35);
        } else {
            bgColor = QColor(0x1a, 0x1b, 0x26, 0);
        }

        painter->setPen(Qt::NoPen);
        painter->setBrush(bgColor);
        painter->drawRoundedRect(rect, 8, 8);


        if (option.state & (QStyle::State_Selected | QStyle::State_MouseOver)) {
            painter->setPen(QPen(QColor(0x3b, 0x42, 0x61), 1));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect, 8, 8);
        }


        QString name = index.data(NameRole).toString();
        QString version = index.data(VersionRole).toString();
        QString loader = index.data(LoaderRole).toString();
        QString instanceId = index.data(IdRole).toString();

        QString iconKey = index.data(IconKeyRole).toString();

        if (SettingsManager::instance().zenMode()) {
            QString text = QStringLiteral("%1%2 [%3] [%4]")
                    .arg(runningInstances_.contains(instanceId) ? QStringLiteral("* ") : QString())
                    .arg(name)
                    .arg(version)
                    .arg(loader.isEmpty() ? QStringLiteral("Vanilla") : loader);

            painter->setPen(option.state & QStyle::State_Selected ? QColor("#5fd7af") : QColor("#b0b0b0"));
            painter->setFont(option.font);
            painter->drawText(rect.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
            painter->restore();
            return;
        }

        QRect iconRect(rect.left() + 20, rect.top() + 12, 48, 48);

        if (iconKey.isEmpty() || iconKey == QStringLiteral("vanilla_icon")) {
            // Fallback or vanilla
            QPixmap pix(QStringLiteral(":/icons/vanilla_icon.png"));
            if (!pix.isNull()) {
                painter->drawPixmap(iconRect, pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                uint hash = qHash(name);
                QColor iconColor = QColor::fromHsl(hash % 360, 140, 100);
                painter->setBrush(iconColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(iconRect, 10, 10);
                painter->setPen(Qt::white);
                QFont iconFont = painter->font();
                iconFont.setPixelSize(22);
                iconFont.setBold(true);
                painter->setFont(iconFont);
                painter->drawText(iconRect, Qt::AlignCenter,
                                  name.isEmpty() ? QStringLiteral("?") : name.left(1).toUpper());
            }
        } else {
            QPixmap pix(QStringLiteral(":/icons/") + iconKey + QStringLiteral(".png"));
            painter->drawPixmap(iconRect, pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }


        if (runningInstances_.contains(instanceId)) {
            painter->setBrush(Theme::Colors::running());
            painter->setPen(QPen(QColor(0x1a, 0x1b, 0x26), 2));
            painter->drawEllipse(QPoint(iconRect.right() - 2, iconRect.top() + 6), 5, 5);
        }


        painter->setPen(QColor(0xc0, 0xca, 0xf5));
        QFont nameFont = option.font;
        nameFont.setPixelSize(12);
        nameFont.setWeight(QFont::Medium);
        painter->setFont(nameFont);

        QRect nameRect(rect.left() + 4, iconRect.bottom() + 6, rect.width() - 8, 18);
        QString elidedName = painter->fontMetrics().elidedText(name, Qt::ElideRight, nameRect.width());
        painter->drawText(nameRect, Qt::AlignHCenter | Qt::AlignTop, elidedName);


        painter->setPen(QColor(0x56, 0x5f, 0x89));
        QFont verFont = option.font;
        verFont.setPixelSize(10);
        painter->setFont(verFont);

        QString subtitle = version;
        if (!loader.isEmpty() && loader != QStringLiteral("Vanilla")) {
            subtitle += QStringLiteral(" • ") + loader;
        }

        QRect verRect(rect.left() + 4, nameRect.bottom(), rect.width() - 8, 14);
        QString elidedVer = painter->fontMetrics().elidedText(subtitle, Qt::ElideRight, verRect.width());
        painter->drawText(verRect, Qt::AlignHCenter | Qt::AlignTop, elidedVer);

        painter->restore();
    }
}