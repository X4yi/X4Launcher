#pragma once

#include <QDialog>
#include <QVector>

class QTableWidget;
class QPushButton;
class QProgressBar;
class QLabel;

namespace x4 {

class JavaManager;

class JavaInstallerDialog : public QDialog {
    Q_OBJECT
public:
    explicit JavaInstallerDialog(JavaManager* javaMgr, QWidget* parent = nullptr);

private slots:
    void onInstallClicked();
    void onSelectionChanged();

private:
    void setupUI();
    void populateTable();

    JavaManager* javaMgr_;
    
    QTableWidget* table_;
    QPushButton* installBtn_;
    QPushButton* closeBtn_;
    QProgressBar* progressBar_;
    QLabel* statusLabel_;

    struct JavaOption {
        int major;
        QString description;
    };
    QVector<JavaOption> options_;
};

} 
