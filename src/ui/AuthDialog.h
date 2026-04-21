#pragma once

#include <QDialog>
#include <QUrl>

class QLabel;
class QListWidget;
class QLineEdit;
class QPushButton;
class QNetworkAccessManager;

namespace x4 {

class AuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuthDialog(QWidget* parent = nullptr);

private slots:
    void startLogin();
    void addOfflineAccount();
    void refreshAccountList();
    void onAccountSelected();
    void removeSelectedAccount();
    
    void onLoginStatus(const QString& status);
    void onLoginFinished(bool success, const QString& message);

private:
    void updateDetails();
    void loadSteveAvatar();

    QListWidget* accountList_;
    QLabel* statusLabel_;
    QLineEdit* offlineNameEdit_;
    QPushButton* addOfflineButton_;
    QPushButton* loginButton_;
    QPushButton* removeButton_;
    QPushButton* activateButton_;
    QNetworkAccessManager* avatarNetwork_;
    QPixmap stevePixmap_;
};

} 
