#pragma once

#include <QListView>
#include <QStandardItemModel>

namespace x4 {

class InstanceManager;
class InstanceDelegate;
struct Instance;


class InstanceListWidget : public QListView {
    Q_OBJECT
public:
    explicit InstanceListWidget(InstanceManager* mgr, QWidget* parent = nullptr);

    
    void refresh();
    void setZenMode(bool zen);
    bool zenMode() const { return zenMode_; }

    void setGroupFilter(const QString& group);
    QStringList availableGroups() const;

    
    QString selectedInstanceId() const;

    
    InstanceDelegate* instanceDelegate() const { return delegate_; }

signals:
    void launchRequested(const QString& instanceId);
    void editRequested(const QString& instanceId);
    void deleteRequested(const QString& instanceId);
    void cloneRequested(const QString& instanceId);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void addInstanceItem(const Instance& inst);
    void applyFilter();

    InstanceManager* mgr_;
    QStandardItemModel* model_;
    InstanceDelegate* delegate_;
    QString currentGroupFilter_;
    bool zenMode_ = false;
};

} 
