#include "ui/theme.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QPushButton>
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

namespace GraphUI {

void applyTheme(QApplication &application) {
  QPalette palette;
  palette.setColor(QPalette::Window, Canvas);
  palette.setColor(QPalette::WindowText, Text);
  palette.setColor(QPalette::Base, Surface);
  palette.setColor(QPalette::AlternateBase, BlueSoft);
  palette.setColor(QPalette::ToolTipBase, Surface);
  palette.setColor(QPalette::ToolTipText, Text);
  palette.setColor(QPalette::Text, Text);
  palette.setColor(QPalette::Button, Surface);
  palette.setColor(QPalette::ButtonText, Text);
  palette.setColor(QPalette::Highlight, Blue);
  palette.setColor(QPalette::HighlightedText, QColor(Qt::white));
  palette.setColor(QPalette::PlaceholderText, Muted);
  palette.setColor(QPalette::Disabled, QPalette::Text, Muted);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, Muted);
  application.setPalette(palette);
  QFont appFont = application.font();
  appFont.setPointSize(12);
  application.setFont(appFont);
  application.setStyleSheet(QString::fromLatin1(R"QSS(
    QMainWindow { background: #FAFCFF; color: #1C2B43; }
    QWidget#appRoot, QWidget#contentArea, QStackedWidget#pageStack,
    QWidget#overviewPage { background: #FAFCFF; color: #1C2B43; }
    QWidget { color: #1C2B43; }
    QLabel { background: transparent; }

    QFrame#sidebar {
      background: rgba(230, 240, 255, 0.78);
      border: 1px solid #D9E5F4;
      border-radius: 14px;
    }
    QWidget#toolbar, QWidget#graphToolbar, QWidget#topToolbar {
      background: rgba(255, 255, 255, 0.88);
      border: 1px solid #D9E5F4;
      border-radius: 12px;
    }
    QLabel#brandTitle { color: #1C2B43; font-size: 21px; font-weight: 700; }
    QLabel#brandCaption, QLabel#sectionCaption, QLabel#eyebrow {
      color: #64758B; font-size: 10px; font-weight: 700; letter-spacing: 1px;
    }
    QLabel#sectionCaption { padding-top: 7px; }
    QLabel#sidebarNote { color: #64758B; font-size: 11px; padding: 8px 3px 0; }
    QLabel[role="hint"] { color: #64758B; }
    QLabel[role="status"] {
      color: #205FC2; background: #E6F0FF; border: 1px solid #CDE0FF;
      border-radius: 8px; padding: 6px 9px;
    }
    QLabel#pageTitle { color: #1C2B43; font-size: 15px; font-weight: 650; }
    QLabel#welcomeTitle { color: #1C2B43; font-size: 29px; font-weight: 700; }
    QLabel#welcomeDescription { color: #64758B; font-size: 14px; }
    QLabel#sectionTitle { color: #1C2B43; font-size: 18px; font-weight: 650; padding-top: 5px; }

    QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit, QTimeEdit,
    QDateTimeEdit {
      background: #FFFFFF; color: #1C2B43; border: 1px solid #D9E5F4;
      border-radius: 8px; padding: 7px 9px; min-height: 20px;
      selection-background-color: #2670E8; selection-color: white;
    }
    QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus,
    QDateEdit:focus, QTimeEdit:focus, QDateTimeEdit:focus,
    QListWidget#navigation:focus { border-color: #2670E8; }
    QComboBox::drop-down { border: none; width: 24px; }
    QComboBox QAbstractItemView {
      background: #FFFFFF; color: #1C2B43; border: 1px solid #D9E5F4;
      selection-background-color: #E6F0FF; selection-color: #1C2B43;
    }
    QCheckBox, QRadioButton { color: #1C2B43; spacing: 7px; }
    QCheckBox::indicator, QRadioButton::indicator { width: 16px; height: 16px; }
    QSlider::groove:horizontal { height: 4px; background: #D9E5F4; border-radius: 2px; }
    QSlider::sub-page:horizontal { background: #2670E8; border-radius: 2px; }
    QSlider::handle:horizontal {
      width: 14px; margin: -5px 0; background: #FFFFFF;
      border: 2px solid #2670E8; border-radius: 8px;
    }
    QProgressBar {
      background: #E6F0FF; color: #1C2B43; border: 1px solid #D9E5F4;
      border-radius: 6px; text-align: center; min-height: 13px;
    }
    QProgressBar::chunk { background: #2670E8; border-radius: 5px; }
    QScrollArea { background: transparent; border: none; }
    QScrollBar:vertical { width: 10px; background: transparent; margin: 2px; }
    QScrollBar::handle:vertical { background: #C8D8ED; min-height: 28px; border-radius: 5px; }
    QScrollBar::handle:vertical:hover { background: #AFC4E0; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QScrollBar:horizontal { height: 10px; background: transparent; margin: 2px; }
    QScrollBar::handle:horizontal { background: #C8D8ED; min-width: 28px; border-radius: 5px; }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
    QMenu, QMenuBar { background: #FFFFFF; color: #1C2B43; }
    QMenu { border: 1px solid #D9E5F4; padding: 5px; }
    QMenu::item { padding: 6px 20px; border-radius: 5px; }
    QMenu::item:selected { background: #E6F0FF; }
    QDialog { background: #FAFCFF; color: #1C2B43; }
    QToolTip { color: #1C2B43; background: #FFFFFF; border: 1px solid #D9E5F4; padding: 5px; }

    QListWidget#navigation { background: transparent; color: #334A66; outline: none; }
    QListWidget#navigation::item {
      min-height: 34px; padding: 2px 8px; border-radius: 7px; margin: 1px 0;
    }
    QListWidget#navigation::item:hover { background: rgba(255, 255, 255, 0.72); }
    QListWidget#navigation::item:selected {
      background: #FFFFFF; color: #205FC2; font-weight: 650;
      border: 1px solid #CDDEF4;
    }
    QFrame#dashboardCard {
      background: #FFFFFF; border: 1px solid #D9E5F4; border-radius: 12px;
    }
    QLabel#cardNumber { color: #2670E8; font-size: 12px; font-weight: 700; }
    QLabel#cardTitle { color: #1C2B43; font-size: 16px; font-weight: 650; }
    QLabel#cardDescription { color: #64758B; font-size: 12px; }
    QPushButton {
      background: #FFFFFF; color: #29415F; border: 1px solid #D1DFF0;
      border-radius: 8px; padding: 7px 12px; min-height: 20px;
    }
    QPushButton:hover { background: #F2F7FF; border-color: #9DBCE9; }
    QPushButton:pressed { background: #E6F0FF; }
    QPushButton:focus { border: 2px solid #2670E8; }
    QPushButton[role="primary"] {
      background: #2670E8; color: #FFFFFF; border-color: #2670E8; font-weight: 600;
    }
    QPushButton[role="primary"]:hover { background: #1F61CC; border-color: #1F61CC; }
    QPushButton[role="primary"]:pressed { background: #194FA9; }
    QPushButton[role="algorithm"]:checked {
      background: #2670E8; color: #FFFFFF; border-color: #2670E8; font-weight: 600;
    }
    QPushButton[role="algorithm"]:checked:hover { background: #1F61CC; border-color: #1F61CC; }
    QPushButton[role="algorithm"]:checked:pressed { background: #194FA9; }
    QPushButton:disabled { background: #F1F4F8; color: #8998AA; border-color: #E0E7EF; }
  )QSS"));
}

QWidget *toolbar(QWidget *parent) {
  auto *bar = new QWidget(parent);
  bar->setObjectName("toolbar");
  bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  auto *layout = new QHBoxLayout(bar);
  layout->setContentsMargins(18, 10, 18, 10);
  layout->setSpacing(10);
  return bar;
}

QPushButton *button(const QString &text, QWidget *parent, bool primary) {
  auto *result = new QPushButton(text, parent);
  result->setCursor(Qt::PointingHandCursor);
  result->setMinimumHeight(36);
  result->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  if (primary)
    result->setProperty("role", QStringLiteral("primary"));
  return result;
}

QLabel *status(const QString &text, QWidget *parent) {
  auto *result = new QLabel(text, parent);
  result->setProperty("role", QStringLiteral("status"));
  result->setAlignment(Qt::AlignCenter);
  result->setWordWrap(true);
  result->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  return result;
}

} // namespace GraphUI
