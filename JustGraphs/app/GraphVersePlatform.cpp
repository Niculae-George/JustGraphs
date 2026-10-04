#include "app/GraphVersePlatform.h"
#include "app/page_registry.h"

#include "ui/theme.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QApplication>

namespace {
QWidget *makeOverviewCard(const QString &number, const QString &title,
                          const QString &description, const QString &pageId,
                          GraphVersePlatform *window) {
  auto *card = new QFrame;
  card->setObjectName("dashboardCard");
  card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  auto *layout = new QVBoxLayout(card);
  layout->setContentsMargins(20, 18, 20, 18);
  layout->setSpacing(10);

  auto *numberLabel = new QLabel(number);
  numberLabel->setObjectName("cardNumber");
  auto *titleLabel = new QLabel(title);
  titleLabel->setObjectName("cardTitle");
  titleLabel->setWordWrap(true);
  auto *descriptionLabel = new QLabel(description);
  descriptionLabel->setObjectName("cardDescription");
  descriptionLabel->setWordWrap(true);
  auto *open = GraphUI::button("Open visualizer  →", card, true);
  open->setObjectName("cardAction");
  open->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

  layout->addWidget(numberLabel);
  layout->addWidget(titleLabel);
  layout->addWidget(descriptionLabel, 1);
  layout->addSpacing(4);
  layout->addWidget(open, 0, Qt::AlignLeft);
  QObject::connect(open, &QPushButton::clicked, window,
                   [window, pageId]() {
                     if (auto *navigation = window->findChild<QListWidget *>("navigation"))
                       for (int i = 0; i < navigation->count(); ++i)
                         if (navigation->item(i)->data(Qt::UserRole + 1).toString() == pageId) {
                           navigation->setCurrentRow(i);
                           break;
                         }
                   });
  return card;
}
} // namespace

GraphVersePlatform::GraphVersePlatform(QWidget *parent) : QMainWindow(parent) {
  GraphUI::applyTheme(*qApp);
  setupUi();
  setWindowTitle("JustGraphs");
  resize(1180, 780);
  setMinimumSize(1120, 700);
}

void GraphVersePlatform::setupUi() {
  const auto& pages = pageDescriptors();
  const int visualizerCount = int(pages.size());
  auto *root = new QWidget(this);
  root->setObjectName("appRoot");
  auto *rootLayout = new QHBoxLayout(root);
  rootLayout->setContentsMargins(16, 16, 16, 16);
  rootLayout->setSpacing(14);
  setCentralWidget(root);

  auto *sidebar = new QFrame(root);
  sidebar->setObjectName("sidebar");
  sidebar->setFixedWidth(240);
  auto *sideLayout = new QVBoxLayout(sidebar);
  sideLayout->setContentsMargins(14, 20, 14, 16);
  sideLayout->setSpacing(12);

  auto *brand = new QLabel("JustGraphs");
  brand->setObjectName("brandTitle");
  auto *brandCaption = new QLabel("GRAPH ALGORITHM STUDIO");
  brandCaption->setObjectName("brandCaption");
  sideLayout->addWidget(brand);
  sideLayout->addWidget(brandCaption);
  sideLayout->addSpacing(12);

  m_search = new QLineEdit(sidebar);
  m_search->setObjectName("navigationSearch");
  m_search->setPlaceholderText("Search visualizers");
  m_search->setClearButtonEnabled(true);
  sideLayout->addWidget(m_search);

  auto *sectionLabel = new QLabel("WORKSPACE");
  sectionLabel->setObjectName("sectionCaption");
  sideLayout->addWidget(sectionLabel);

  m_navigation = new QListWidget(sidebar);
  m_navigation->setObjectName("navigation");
  m_navigation->setAccessibleName("Visualizer navigation");
  m_navigation->setFrameShape(QFrame::NoFrame);
  m_navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_navigation->setSpacing(3);
  m_navigation->addItem(new QListWidgetItem("⌂   Overview"));
  for (int index = 0; index < visualizerCount; ++index) {
    const auto& page = pages[size_t(index)];
    const QString number = QString::number(index + 1).rightJustified(2, '0');
    auto *item = new QListWidgetItem(QString("%1   %2").arg(number, page.navigationLabel));
    item->setToolTip(page.title);
    item->setData(Qt::UserRole, page.keywords + " " + page.title);
    item->setData(Qt::UserRole + 1, page.id);
    m_navigation->addItem(item);
  }
  sideLayout->addWidget(m_navigation, 1);

  auto *sideNote = new QLabel("Interactive graph algorithm visualizations");
  sideNote->setObjectName("sidebarNote");
  sideNote->setWordWrap(true);
  sideLayout->addWidget(sideNote);

  auto *content = new QWidget(root);
  content->setObjectName("contentArea");
  auto *contentLayout = new QVBoxLayout(content);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(12);

  auto *topbar = GraphUI::toolbar(content);
  topbar->setObjectName("topToolbar");
  auto *topLayout = qobject_cast<QHBoxLayout *>(topbar->layout());
  m_pageTitle = new QLabel("Overview", topbar);
  m_pageTitle->setObjectName("pageTitle");
  topLayout->addWidget(m_pageTitle);
  topLayout->addStretch();
  auto *status = GraphUI::status(QStringLiteral("%1 visualizers").arg(visualizerCount), topbar);
  status->setObjectName("visualizerCount");
  status->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  topLayout->addWidget(status);
  contentLayout->addWidget(topbar);

  m_pages = new QStackedWidget(content);
  m_pages->setObjectName("pageStack");
  contentLayout->addWidget(m_pages, 1);

  auto *overviewScroll = new QScrollArea(m_pages);
  overviewScroll->setObjectName("overviewScroll");
  overviewScroll->setWidgetResizable(true);
  overviewScroll->setFrameShape(QFrame::NoFrame);
  auto *overview = new QWidget;
  overview->setObjectName("overviewPage");
  auto *overviewLayout = new QVBoxLayout(overview);
  overviewLayout->setContentsMargins(8, 12, 8, 24);
  overviewLayout->setSpacing(16);

  auto *eyebrow = new QLabel("GRAPH THEORY · VISUAL LEARNING");
  eyebrow->setObjectName("eyebrow");
  auto *welcome = new QLabel("See graph algorithms in action.");
  welcome->setObjectName("welcomeTitle");
  welcome->setWordWrap(true);
  auto *intro = new QLabel("Choose a visualizer to explore how classic algorithms work, step by step.");
  intro->setObjectName("welcomeDescription");
  intro->setWordWrap(true);
  overviewLayout->addWidget(eyebrow);
  overviewLayout->addWidget(welcome);
  overviewLayout->addWidget(intro);
  overviewLayout->addSpacing(10);

  auto *visualizersTitle = new QLabel("All visualizers");
  visualizersTitle->setObjectName("sectionTitle");
  overviewLayout->addWidget(visualizersTitle);

  auto *cards = new QWidget(overview);
  auto *grid = new QGridLayout(cards);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setHorizontalSpacing(12);
  grid->setVerticalSpacing(12);
  grid->setColumnStretch(0, 1);
  grid->setColumnStretch(1, 1);
  for (int i = 0; i < visualizerCount; ++i) {
    const int row = i / 2;
    const int col = i % 2;
    grid->addWidget(makeOverviewCard(QString::number(i + 1).rightJustified(2, '0'),
                                     pages[size_t(i)].title,
                                     pages[size_t(i)].subtitle,
                                     pages[size_t(i)].id, this), row, col);
  }
  overviewLayout->addWidget(cards);
  overviewLayout->addStretch();
  overviewScroll->setWidget(overview);
  m_pages->addWidget(overviewScroll);

  for (const auto& descriptor : pages) {
    auto* page = descriptor.create(m_pages);
    page->setProperty("pageId", descriptor.id);
    page->setProperty("pageTitle", descriptor.title);
    m_pages->addWidget(page);
  }

  rootLayout->addWidget(sidebar);
  rootLayout->addWidget(content, 1);

  connect(m_navigation, &QListWidget::currentRowChanged, this,
          [this](int row) { selectPage(row); });
  connect(m_search, &QLineEdit::textChanged, this, [this](const QString &query) {
    const QString needle = query.trimmed();
    for (int row = 0; row < m_navigation->count(); ++row) {
      auto *item = m_navigation->item(row);
      const QString keywords = item->data(Qt::UserRole).toString();
      item->setHidden(!item->text().contains(needle, Qt::CaseInsensitive) &&
                      !keywords.contains(needle, Qt::CaseInsensitive));
    }
  });
  m_navigation->setCurrentRow(0);
}

void GraphVersePlatform::selectPage(int index) {
  if (index < 0 || index >= m_pages->count())
    return;
  m_pages->setCurrentIndex(index);
  m_pageTitle->setText(index == 0 ? QStringLiteral("Overview")
                                  : m_pages->currentWidget()->property("pageTitle").toString());
}
