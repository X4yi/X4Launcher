#include "Theme.h"
#include <QFile>
#include <QFont>
#include <QFontDatabase>

namespace x4 {
    void Theme::apply(const QString &themeName) {
        if (themeName == QStringLiteral("light"))
            applyLight();
        else if (themeName == QStringLiteral("zen"))
            applyZen();
        else
            applyDark();
    }

    void Theme::applyDark() {
        QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-Regular.ttf"));

        QString qss = QStringLiteral(R"(
* {
    font-family: "Inter", "Segoe UI", "Noto Sans", sans-serif;
    font-size: 13px;
}

QMainWindow, QDialog {
    background-color: #1a1b26;
    color: #c0caf5;
}

QToolBar {
    background-color: #1f2335;
    border: none;
    padding: 4px;
    spacing: 4px;
}

QToolBar::separator {
    width: 1px;
    background-color: #3b4261;
    margin: 4px 8px;
}

QToolButton {
    background-color: transparent;
    color: #c0caf5;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 6px 12px;
    font-weight: 500;
}

QToolButton:hover {
    background-color: #292e42;
    border-color: #3b4261;
}

QToolButton:pressed {
    background-color: #343b58;
}

QListView, QTreeView, QTableView {
    background-color: #1a1b26;
    alternate-background-color: #1f2335;
    color: #c0caf5;
    border: 1px solid #292e42;
    border-radius: 8px;
    outline: none;
    padding: 4px;
}

QListView::item, QTreeView::item {
    padding: 4px;
    border-radius: 6px;
}

QListView::item:selected, QTreeView::item:selected {
    background-color: #283457;
    color: #c0caf5;
}

QListView::item:hover, QTreeView::item:hover {
    background-color: #1f2335;
}

QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background-color: #3b4261;
    border-radius: 4px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background-color: #545c7e;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
    height: 0;
}

QScrollBar:horizontal {
    background: transparent;
    height: 8px;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background-color: #3b4261;
    border-radius: 4px;
    min-width: 30px;
}

QScrollBar::handle:horizontal:hover {
    background-color: #545c7e;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: none;
    width: 0;
}

/* --- Tab Widget --- */
QTabWidget::pane {
    background-color: #1a1b26;
    border: 1px solid #292e42;
    border-radius: 6px;
    top: -1px;
}

QTabBar::tab {
    background-color: #1f2335;
    color: #565f89;
    border: none;
    padding: 8px 16px;
    margin-right: 2px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}

QTabBar::tab:selected {
    background-color: #1a1b26;
    color: #c0caf5;
    border-bottom: 2px solid #7aa2f7;
}

QTabBar::tab:hover:!selected {
    background-color: #292e42;
    color: #a9b1d6;
}

/* --- Buttons --- */
QPushButton {
    background-color: #292e42;
    color: #c0caf5;
    border: 1px solid #3b4261;
    border-radius: 6px;
    padding: 6px 16px;
    font-weight: 500;
}

QPushButton:hover {
    background-color: #343b58;
    border-color: #545c7e;
}

QPushButton:pressed {
    background-color: #1f2335;
}

QPushButton:disabled {
    color: #565f89;
    background-color: #1f2335;
    border-color: #292e42;
}

QPushButton#launchButton {
    background-color: #283457;
    color: #7aa2f7;
    border-color: #394b70;
    font-weight: 600;
}

QPushButton#launchButton:hover {
    background-color: #394b70;
    border-color: #7aa2f7;
}

/* --- Input Fields --- */
QLineEdit, QSpinBox, QComboBox {
    background-color: #1f2335;
    color: #c0caf5;
    border: 1px solid #3b4261;
    border-radius: 6px;
    padding: 6px 10px;
    selection-background-color: #283457;
}

QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
    border-color: #7aa2f7;
}

QComboBox::drop-down {
    border: none;
    width: 24px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #565f89;
    margin-right: 8px;
}

QComboBox QAbstractItemView {
    background-color: #1f2335;
    color: #c0caf5;
    border: 1px solid #3b4261;
    border-radius: 4px;
    selection-background-color: #283457;
}

/* --- Text Edit (Console) --- */
QPlainTextEdit {
    background-color: #111320;
    color: #a9b1d6;
    border: 1px solid #292e42;
    border-radius: 6px;
    font-family: "JetBrains Mono", "Cascadia Code", "Fira Code", "Consolas", monospace;
    font-size: 12px;
    padding: 8px;
    selection-background-color: #283457;
}

/* --- Labels --- */
QLabel {
    color: #c0caf5;
}

QLabel#titleLabel {
    font-size: 15px;
    font-weight: 600;
    color: #c0caf5;
}

QLabel#subtitleLabel {
    font-size: 11px;
    color: #565f89;
}

/* --- Group Box --- */
QGroupBox {
    color: #c0caf5;
    border: 1px solid #292e42;
    border-radius: 8px;
    margin-top: 12px;
    padding-top: 16px;
    font-weight: 500;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 6px;
    color: #7aa2f7;
}

/* --- Status Bar --- */
QStatusBar {
    background-color: #1f2335;
    color: #565f89;
    border-top: 1px solid #292e42;
}

QStatusBar::item {
    border: none;
}

/* --- Menu --- */
QMenuBar {
    background-color: #1f2335;
    color: #c0caf5;
    border-bottom: 1px solid #292e42;
}

QMenuBar::item:selected {
    background-color: #292e42;
}

QMenu {
    background-color: #1f2335;
    color: #c0caf5;
    border: 1px solid #3b4261;
    border-radius: 6px;
    padding: 4px;
}

QMenu::item {
    padding: 6px 24px 6px 12px;
    border-radius: 4px;
}

QMenu::item:selected {
    background-color: #283457;
}

QMenu::separator {
    height: 1px;
    background-color: #3b4261;
    margin: 4px 8px;
}

/* --- Progress Bar --- */
QProgressBar {
    background-color: #1f2335;
    border: 1px solid #292e42;
    border-radius: 4px;
    text-align: center;
    color: #c0caf5;
    height: 6px;
}

QProgressBar::chunk {
    background-color: #7aa2f7;
    border-radius: 3px;
}

/* --- Checkbox / Radio --- */
QCheckBox, QRadioButton {
    color: #c0caf5;
    spacing: 8px;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid #3b4261;
    background-color: #1f2335;
}

QCheckBox::indicator {
    border-radius: 4px;
}

QRadioButton::indicator {
    border-radius: 8px;
}

QCheckBox::indicator:checked {
    background-color: #7aa2f7;
    border-color: #7aa2f7;
}

QRadioButton::indicator:checked {
    background-color: #7aa2f7;
    border-color: #7aa2f7;
}

/* --- Splitter --- */
QSplitter::handle {
    background-color: #292e42;
    width: 1px;
    height: 1px;
}

/* --- Tooltip --- */
QToolTip {
    background-color: #1f2335;
    color: #c0caf5;
    border: 1px solid #3b4261;
    border-radius: 4px;
    padding: 4px 8px;
}
    )");

        qApp->setStyleSheet(qss);
    }

    void Theme::applyLight() {
        QString qss = QStringLiteral(R"(
/* ===== X4Launcher Light Theme ===== */

* {
    font-family: "Inter", "Segoe UI", "Noto Sans", sans-serif;
    font-size: 13px;
}

QMainWindow, QDialog {
    background-color: #f0f0f5;
    color: #1a1b26;
}

QToolBar {
    background-color: #ffffff;
    border: none;
    border-bottom: 1px solid #e0e0e8;
    padding: 4px;
    spacing: 4px;
}

QToolButton {
    background-color: transparent;
    color: #1a1b26;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 6px 12px;
}

QToolButton:hover {
    background-color: #e8e8f0;
    border-color: #d0d0d8;
}

QListView, QTreeView, QTableView {
    background-color: #ffffff;
    alternate-background-color: #f8f8fc;
    color: #1a1b26;
    border: 1px solid #e0e0e8;
    border-radius: 8px;
}

QListView::item:selected {
    background-color: #dce4f8;
    color: #1a1b26;
}

QPushButton {
    background-color: #ffffff;
    color: #1a1b26;
    border: 1px solid #d0d0d8;
    border-radius: 6px;
    padding: 6px 16px;
}

QPushButton:hover {
    background-color: #f0f0f5;
    border-color: #b0b0b8;
}

QPushButton#launchButton {
    background-color: #dce4f8;
    color: #3868c8;
    border-color: #b8c8e8;
}

QLineEdit, QSpinBox, QComboBox {
    background-color: #ffffff;
    color: #1a1b26;
    border: 1px solid #d0d0d8;
    border-radius: 6px;
    padding: 6px 10px;
}

QPlainTextEdit {
    background-color: #fafafe;
    color: #1a1b26;
    border: 1px solid #e0e0e8;
    border-radius: 6px;
    font-family: "JetBrains Mono", "Cascadia Code", monospace;
    font-size: 12px;
    padding: 8px;
}

QTabWidget::pane {
    background-color: #f0f0f5;
    border: 1px solid #e0e0e8;
    border-radius: 6px;
}

QTabBar::tab {
    background-color: #e8e8f0;
    color: #888898;
    padding: 8px 16px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}

QTabBar::tab:selected {
    background-color: #f0f0f5;
    color: #1a1b26;
    border-bottom: 2px solid #3868c8;
}

QStatusBar {
    background-color: #ffffff;
    color: #888898;
    border-top: 1px solid #e0e0e8;
}

QScrollBar:vertical {
    background: transparent;
    width: 8px;
}
QScrollBar::handle:vertical {
    background-color: #d0d0d8;
    border-radius: 4px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover {
    background-color: #b0b0b8;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none; height: 0;
}
    )");

        qApp->setStyleSheet(qss);
    }

    void Theme::applyZen() {
        QString qss = QStringLiteral(R"(
QWidget {
    background-color: #0c0c0c;
    color: #c0c0c0;
    border: none;
}

QMainWindow, QDialog, QWidget {
    background-color: #0c0c0c;
    color: #c0c0c0;
}

QListView, QTreeView, QTableView, QPlainTextEdit {
    background-color: #0c0c0c;
    color: #c0c0c0;
    border: none;
}

QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {
    background-color: #1b1b1b;
    color: #79d7af;
}

QPushButton {
    background-color: transparent;
    color: #c0c0c0;
    border: 1px solid #2c2c2c;
    border-radius: 4px;
    padding: 4px 10px;
}

QPushButton:hover {
    background-color: #1f1f1f;
}

QPushButton:pressed {
    background-color: #111111;
}

QComboBox, QLineEdit, QSpinBox {
    background-color: #0c0c0c;
    color: #c0c0c0;
    border: none;
    border-radius: 0px;
    padding: 4px;
}

QPushButton:hover {
    background-color: #1e1e1e;
}

QScrollBar:vertical, QScrollBar:horizontal {
    background: transparent;
    width: 8px;
    height: 8px;
}

QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background-color: #333333;
    border-radius: 4px;
    min-height: 20px;
    min-width: 20px;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: none;
    width: 0;
    height: 0;
}
)");

        qApp->setStyleSheet(qss);
    }
}