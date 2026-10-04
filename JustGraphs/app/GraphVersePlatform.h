#pragma once


#include <QMainWindow>

class QListWidget;
class QLineEdit;
class QLabel;
class QStackedWidget;

class GraphVersePlatform : public QMainWindow {
  Q_OBJECT

public:
  explicit GraphVersePlatform(QWidget *parent = nullptr);
  ~GraphVersePlatform() override = default;

private:
  void setupUi();
  void selectPage(int index);

  QListWidget *m_navigation = nullptr;
  QLineEdit *m_search = nullptr;
  QLabel *m_pageTitle = nullptr;
  QStackedWidget *m_pages = nullptr;
};
