#pragma once

#include <QColor>
#include <QString>

class QApplication;
class QLabel;
class QPushButton;
class QWidget;

namespace GraphUI {
inline const QColor Canvas{250, 252, 255};
inline const QColor Surface{255, 255, 255};
inline const QColor Blue{38, 112, 232};
inline const QColor BlueSoft{230, 240, 255};
inline const QColor Text{28, 43, 67};
inline const QColor Muted{100, 117, 139};
inline const QColor Border{217, 229, 244};
inline const QColor Edge{151, 172, 202};
inline const QColor Success{27, 139, 103};
inline const QColor Warning{182, 114, 23};
inline const QColor Error{201, 68, 78};

void applyTheme(QApplication& application);
QWidget* toolbar(QWidget* parent);
QPushButton* button(const QString& text, QWidget* parent, bool primary = false);
QLabel* status(const QString& text, QWidget* parent);
} // namespace GraphUI
