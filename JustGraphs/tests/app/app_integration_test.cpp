#include "app/GraphVersePlatform.h"
#include "app/page_registry.h"
#include "features/flow/vis_flow.h"
#include "features/components/vis_components.h"
#include "features/topological/vis_topological.h"
#include "features/articulation/vis_artbridge.h"
#include "features/flood_fill/vis_floodfill.h"
#include "features/maze/vis_maze.h"
#include "features/mst/vis_mst.h"
#include "features/shortest_paths/vis_shortest_paths_page.h"
#include "features/spanning_tree/vis_spanning_tree.h"
#include "ui/theme.h"

#include <QApplication>
#include <QCheckBox>
#include <QFrame>
#include <QImage>
#include <QLineF>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSlider>
#include <QSet>
#include <QScreen>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QWheelEvent>
#include <QtTest>

namespace {
struct CurrentDirectoryGuard {
    QString original = QDir::currentPath();
    ~CurrentDirectoryGuard() { QDir::setCurrent(original); }
};

QPushButton *button(QWidget *root, const char *name) {
    return root->findChild<QPushButton *>(QString::fromLatin1(name));
}

void rightClick(QWidget *widget, QPoint point) {
    QTest::mouseClick(widget, Qt::RightButton, Qt::NoModifier, point);
}

void click(QWidget *widget, QPoint point) {
    QTest::mouseClick(widget, Qt::LeftButton, Qt::NoModifier, point);
}

void drag(QWidget *widget, QPoint from, QPoint to) {
    QTest::mousePress(widget, Qt::LeftButton, Qt::NoModifier, from);
    QMouseEvent move(QEvent::MouseMove, QPointF(to), QPointF(widget->mapToGlobal(to)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &move);
    QTest::mouseRelease(widget, Qt::LeftButton, Qt::NoModifier, to);
}

void captureIfRequested(QWidget *widget, const QString &name) {
    const QString directory = qEnvironmentVariable("JUSTGRAPHS_SCREENSHOT_DIR");
    if (directory.isEmpty()) return;
    QDir().mkpath(directory);
    widget->grab().save(QDir(directory).filePath(name));
}

QPushButton *buttonWithText(QWidget *root, const QString &text) {
    for (auto *candidate : root->findChildren<QPushButton *>())
        if (candidate->text() == text) return candidate;
    return nullptr;
}

void addGraphNodes(QWidget *widget, const QList<QPoint> &points) {
    for (const QPoint &point : points)
        rightClick(widget, point);
}

void addFlowNode(NetworkWidget *canvas, QPoint point) {
    rightClick(canvas, point);
}

void clickFlowNode(NetworkWidget *canvas, QPoint point) {
    click(canvas, point);
}

int edgeCount(const Graph *graph) {
    return graph ? int(graph->getEdges().size()) : -1;
}

bool hasEdge(const Graph *graph, int from, int to) {
    if (!graph) return false;
    for (const auto &edge : graph->getEdges())
        if (edge.getFirst().getIndex() == from && edge.getSecond().getIndex() == to)
            return true;
    return false;
}

void connectGraphNodes(QWidget *page, QPoint from, QPoint to) {
    click(page, from);
    click(page, to);
}
}

class AppIntegrationTest final : public QObject {
    Q_OBJECT
private slots:
    void shellBuildsRegistryPagesAndOverviewCards() {
        const auto &descriptors = pageDescriptors();
        QCOMPARE(descriptors.size(), size_t(9));
        QSet<QString> ids;
        for (const auto &descriptor : descriptors) {
            QVERIFY(!descriptor.id.isEmpty());
            QVERIFY(!descriptor.title.isEmpty());
            QVERIFY(!descriptor.navigationLabel.isEmpty());
            QVERIFY(descriptor.create != nullptr);
            QVERIFY(!ids.contains(descriptor.id));
            ids.insert(descriptor.id);
        }

        GraphVersePlatform shell;
        shell.show();
        QTRY_VERIFY(shell.isVisible());
        auto *navigation = shell.findChild<QListWidget *>("navigation");
        auto *stack = shell.findChild<QStackedWidget *>("pageStack");
        auto *title = shell.findChild<QLabel *>("pageTitle");
        QVERIFY(navigation);
        QVERIFY(stack);
        QVERIFY(title);
        QCOMPARE(navigation->count(), 10);
        QCOMPARE(stack->count(), 10);
        QCOMPARE(title->text(), QStringLiteral("Overview"));
        QCOMPARE(shell.findChildren<QFrame *>("dashboardCard").size(), 9);

        for (int index = 0; index < 9; ++index) {
            const auto &descriptor = descriptors[size_t(index)];
            QCOMPARE(navigation->item(index + 1)->data(Qt::UserRole + 1).toString(), descriptor.id);
            QCOMPARE(stack->widget(index + 1)->property("pageId").toString(), descriptor.id);
            QCOMPARE(stack->widget(index + 1)->property("pageTitle").toString(), descriptor.title);
            navigation->setCurrentRow(index + 1);
            QCOMPARE(stack->currentIndex(), index + 1);
            QCOMPARE(title->text(), descriptor.title);
        }
        navigation->setCurrentRow(0);
        QCOMPARE(title->text(), QStringLiteral("Overview"));

        auto *search = shell.findChild<QLineEdit *>("navigationSearch");
        QVERIFY(search);
        search->setText(QStringLiteral("Tarjan"));
        QCoreApplication::processEvents();
        QCOMPARE(navigation->item(9)->isHidden(), false);
        QCOMPARE(navigation->item(1)->isHidden(), true);
        search->setText(QStringLiteral("not-a-page"));
        QCoreApplication::processEvents();
        for (int row = 1; row < navigation->count(); ++row)
            QVERIFY(navigation->item(row)->isHidden());
        search->clear();
        for (int row = 1; row < navigation->count(); ++row)
            QVERIFY(!navigation->item(row)->isHidden());

        // Overview card actions resolve page IDs through the same registry data.
        const auto cards = shell.findChildren<QFrame *>("dashboardCard");
        for (int index = 0; index < cards.size(); ++index) {
            auto *action = button(cards[index], "cardAction");
            QVERIFY(action);
            action->click();
            QCOMPARE(stack->currentIndex(), index + 1);
            QCOMPARE(navigation->currentRow(), index + 1);
        }
    }

    void flowCanvasEditsNetworkAndInvalidatesSolver() {
        VisFlow page;
        page.resize(1000, 700);
        page.show();
        QTRY_VERIFY(page.canvas()->isVisible());
        auto *canvas = page.canvas();
        auto *network = page.network();
        QCOMPARE(network->getNumNodes(), 0);
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::Edit);

        const QPoint a(130, 120), b(350, 120), c(570, 120);
        addFlowNode(canvas, a);
        addFlowNode(canvas, b);
        addFlowNode(canvas, c);
        QCOMPARE(network->getNumNodes(), 3);
        QCOMPARE(network->getNodePositions()[0], QPointF(a));
        QCOMPARE(network->getNodePositions()[1], QPointF(b));
        QCOMPARE(network->getNodePositions()[2], QPointF(c));

        // Selecting two nodes opens edge editor; applying capacity and cost changes graph data.
        clickFlowNode(canvas, a);
        clickFlowNode(canvas, b);
        QVERIFY(canvas->edgeEditorVisible());
        QWidget *editor = canvas->edgeEditorWidget();
        auto *capacity = editor->findChild<QLineEdit *>("flowCapacity");
        auto *cost = editor->findChild<QLineEdit *>("flowCost");
        auto *apply = button(editor, "flowApplyEdge");
        QVERIFY(capacity && cost && apply);
        capacity->setText(QStringLiteral("7"));
        cost->setText(QStringLiteral("3"));
        apply->click();
        QCOMPARE(network->getCapacity(1, 2), 7);
        QCOMPARE(network->getEdgeCost(1, 2), 3);
        QVERIFY(!canvas->edgeEditorVisible());

        // Existing edge can be reopened and edited.
        clickFlowNode(canvas, a);
        clickFlowNode(canvas, b);
        QVERIFY(canvas->edgeEditorVisible());
        QCOMPARE(capacity->text(), QStringLiteral("7"));
        capacity->setText(QStringLiteral("0"));
        apply->click();
        QCOMPARE(network->getCapacity(1, 2), 7);
        QVERIFY(canvas->edgeEditorVisible());
        capacity->setText(QStringLiteral("11"));
        cost->setText(QStringLiteral("-2"));
        apply->click();
        QCOMPARE(network->getCapacity(1, 2), 11);
        QCOMPARE(network->getEdgeCost(1, 2), -2);

        // Connect a second edge and select endpoints from their toolbar modes.
        clickFlowNode(canvas, b);
        clickFlowNode(canvas, c);
        QVERIFY(canvas->edgeEditorVisible());
        capacity->setText(QStringLiteral("5"));
        cost->setText(QStringLiteral("0"));
        apply->click();
        QCOMPARE(network->getCapacity(2, 3), 5);
        button(&page, "flowSource")->click();
        clickFlowNode(canvas, a);
        QCOMPARE(network->getSource(), 1);
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::Edit);
        button(&page, "flowSink")->click();
        clickFlowNode(canvas, c);
        QCOMPARE(network->getSink(), 3);
        QVERIFY(network->hasValidEndpoints());
        captureIfRequested(&page, QStringLiteral("flow.png"));

        // Starting then editing an edge resets algorithm output and returns editing mode.
        button(&page, "flowStart")->click();
        QCOMPARE(canvas->getIterationIndex(), 0);
        QVERIFY(button(&page, "flowNext")->isEnabled());
        button(&page, "flowNext")->click();
        QVERIFY(page.solver()->iterations().size() >= size_t(1));
        button(&page, "flowReset")->click();
        QCOMPARE(canvas->getIterationIndex(), -1);
        button(&page, "flowEdit")->click();
        clickFlowNode(canvas, a);
        clickFlowNode(canvas, b);
        QVERIFY(canvas->edgeEditorVisible());
        capacity->setText(QStringLiteral("13"));
        cost->setText(QStringLiteral("-1"));
        apply->click();
        QCOMPARE(network->getCapacity(1, 2), 13);
        QCOMPARE(canvas->getIterationIndex(), -1);

        // Dragging clamps node centers into visible canvas; deleting selected node repairs endpoints.
        const QPointF before = network->getNodePositions()[1];
        drag(canvas, before.toPoint(), QPoint(canvas->width() + 300, canvas->height() + 300));
        const QPointF moved = network->getNodePositions()[1];
        QVERIFY(moved != before);
        QVERIFY(moved.x() <= canvas->width() - 20);
        QVERIFY(moved.y() <= canvas->height() - 20);

        button(&page, "flowEdit")->click();
        clickFlowNode(canvas, moved.toPoint());
        QCOMPARE(canvas->selectedNode(), 2);
        button(&page, "flowDelete")->click();
        QCOMPARE(network->getNumNodes(), 2);
        QCOMPARE(network->getSink(), 2);
        QVERIFY(network->hasValidEndpoints());
    }

    void flowEndpointValidationAndHideCleanup() {
        VisFlow page;
        page.resize(900, 620);
        page.show();
        auto *canvas = page.canvas();
        addFlowNode(canvas, QPoint(100, 100));
        addFlowNode(canvas, QPoint(300, 100));
        button(&page, "flowStart")->click();
        QVERIFY(page.findChild<QLabel *>("flowStatus")->text().contains(QStringLiteral("source"), Qt::CaseInsensitive));
        button(&page, "flowSource")->click();
        clickFlowNode(canvas, QPoint(100, 100));
        button(&page, "flowSink")->click();
        clickFlowNode(canvas, QPoint(300, 100));
        button(&page, "flowStart")->click();
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::View);
        QCOMPARE(canvas->getIterationIndex(), 0);
        page.hide();
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::View);
        QVERIFY(!canvas->edgeEditorVisible());
    }

    void mapLoadsEmbeddedDataAndKeepsViewStateAcrossAlgorithmChange() {
        CurrentDirectoryGuard cwd;
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QVERIFY(QDir::setCurrent(temporary.path()));
        VisShortestPathsPage page;
        page.resize(1100, 720);
        page.show();
        auto *canvas = page.mapCanvas();
        QTRY_VERIFY(canvas->isVisible());
        QVERIFY(canvas->map());
        QVERIFY(canvas->map()->nodes().size() > 100);
        QCOMPARE(canvas->routeLength(), qint64(-1));
        QCOMPARE(canvas->sourceIndex(), -1);
        QCOMPARE(canvas->targetIndex(), -1);
        QVERIFY(canvas->zoom() > 0.0);
        captureIfRequested(&page, QStringLiteral("luxembourg-map.png"));

        QVERIFY(canvas->setEndpoints(0, 10000));
        const int source = canvas->sourceIndex();
        const int target = canvas->targetIndex();
        button(&page, "mapZoomIn")->click();
        const qreal zoomed = canvas->zoom();
        QVERIFY(zoomed > 1.0);
        button(&page, "mapAStar")->click();
        QCOMPARE(canvas->selectedAlgorithm(), Luxembourg::Algorithm::AStar);
        QCOMPARE(canvas->sourceIndex(), source);
        QCOMPARE(canvas->targetIndex(), target);
        QVERIFY(qAbs(canvas->zoom() - zoomed) < 0.001);
        QCOMPARE(canvas->routeLength(), qint64(-1));

        auto *speed = page.findChild<QSlider *>("mapSpeed");
        QVERIFY(speed);
        speed->setValue(speed->maximum());
        button(&page, "mapStart")->click();
        button(&page, "mapPause")->click();
        QVERIFY(canvas->paused());
        button(&page, "mapPause")->click();
        QVERIFY(!canvas->paused());
        page.hide();
        QVERIFY(canvas->paused());
        const int settledBeforeShow = canvas->settledShown();
        page.show();
        QTRY_VERIFY(canvas->isVisible());
        QVERIFY(canvas->settledShown() >= settledBeforeShow);
        QVERIFY(canvas->paused());
        button(&page, "mapPause")->click();
        QVERIFY(!canvas->paused());
        QTRY_VERIFY_WITH_TIMEOUT(canvas->routeAttempted(), 2000);
        QTRY_VERIFY_WITH_TIMEOUT(!canvas->routing(), 15000);
        QCOMPARE(canvas->routeLength(), qint64(14733));
        QVERIFY(!canvas->routeResult().nodes.empty());

        const QList<QPair<const char *, Luxembourg::Algorithm>> algorithms{
            {"mapDijkstra", Luxembourg::Algorithm::Dijkstra},
            {"mapAStar", Luxembourg::Algorithm::AStar},
            {"mapBiDijkstra", Luxembourg::Algorithm::BidirectionalDijkstra},
            {"mapBellmanFord", Luxembourg::Algorithm::BellmanFord}
        };
        for (const auto &entry : algorithms) {
            button(&page, entry.first)->click();
            QCOMPARE(canvas->selectedAlgorithm(), entry.second);
            QCOMPARE(canvas->sourceIndex(), source);
            QCOMPARE(canvas->targetIndex(), target);
            QCOMPARE(canvas->routeLength(), qint64(-1));
            QVERIFY(qAbs(canvas->zoom() - zoomed) < 0.001);
            button(&page, "mapStart")->click();
            QTRY_VERIFY_WITH_TIMEOUT(!canvas->routing(), 15000);
            QCOMPARE(canvas->routeLength(), qint64(14733));
            // Selecting already active algorithm must leave computed result intact.
            button(&page, entry.first)->click();
            QCOMPARE(canvas->routeLength(), qint64(14733));
            QCOMPARE(canvas->sourceIndex(), source);
            QCOMPARE(canvas->targetIndex(), target);
        }
        QVERIFY(canvas->setEndpoints(1, 30000));
        button(&page, "mapDijkstra")->click();
        button(&page, "mapStart")->click();
        QTRY_VERIFY_WITH_TIMEOUT(!canvas->routing(), 15000);
        QCOMPARE(canvas->routeLength(), qint64(28814));

        // Mouse drag pans; wheel zooms. Reset restores fit view.
        const QPoint center = canvas->rect().center();
        const QPointF worldBefore = canvas->worldPositionAt(center);
        drag(canvas, center, center + QPoint(45, 20));
        const QPointF worldAfter = canvas->worldPositionAt(center);
        QVERIFY(QLineF(worldBefore, worldAfter).length() > 1.0);
        QWheelEvent wheel(QPointF(center), QPointF(canvas->mapToGlobal(center)), QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(canvas, &wheel);
        QVERIFY(canvas->zoom() > 0.0);
        button(&page, "mapFit")->click();
        QVERIFY(qAbs(canvas->zoom() - 1.0) < 0.01);

    }

    void graphWidgetsAcceptNodeConnectionDragAndClearGestures() {
        // The component page is the representative shared graph-editing surface.
        auto *component = pageDescriptors()[1].create(nullptr);
        component->resize(800, 600);
        component->show();
        addGraphNodes(component, {QPoint(170, 190), QPoint(390, 190), QPoint(610, 190)});
        QImage before(component->size(), QImage::Format_ARGB32_Premultiplied);
        before.fill(Qt::transparent);
        component->render(&before);
        click(component, QPoint(170, 190));
        click(component, QPoint(390, 190));
        QImage after(component->size(), QImage::Format_ARGB32_Premultiplied);
        after.fill(Qt::transparent);
        component->render(&after);
        QVERIFY(before != after);

        drag(component, QPoint(170, 190), QPoint(220, 240));
        QImage moved(component->size(), QImage::Format_ARGB32_Premultiplied);
        moved.fill(Qt::transparent);
        component->render(&moved);
        QVERIFY(after != moved);
        auto *directed = component->findChild<QCheckBox *>("componentsDirected");
        auto *findComponents = button(component, "componentsFind");
        auto *condensed = button(component, "componentsCondensed");
        QVERIFY(directed && findComponents && condensed);
        directed->setChecked(true);
        QVERIFY(findComponents->text().contains(QStringLiteral("strongly connected"), Qt::CaseInsensitive));
        QVERIFY(!condensed->isEnabled());
        QVERIFY(!directed->geometry().intersects(findComponents->geometry()));
        QVERIFY(!findComponents->geometry().intersects(condensed->geometry()));
        captureIfRequested(component, QStringLiteral("components-directed.png"));
        component->hide();
        QVERIFY(component->isHidden());
        component->show();
        QCoreApplication::processEvents();
        QImage afterShow(component->size(), QImage::Format_ARGB32_Premultiplied);
        afterShow.fill(Qt::transparent);
        component->render(&afterShow);
        auto *emptyComponent = pageDescriptors()[1].create(nullptr);
        emptyComponent->resize(component->size());
        emptyComponent->show();
        QImage emptyImage(component->size(), QImage::Format_ARGB32_Premultiplied);
        emptyImage.fill(Qt::transparent);
        emptyComponent->render(&emptyImage);
        QVERIFY(afterShow != emptyImage);
        delete emptyComponent;
        captureIfRequested(component, QStringLiteral("components-after-hide-show.png"));
        delete component;

        // Topological and articulation pages share add/connect/drag interaction.
        QWidget *topological = pageDescriptors()[6].create(nullptr);
        topological->resize(800, 600);
        topological->show();
        addGraphNodes(topological, {QPoint(170, 190), QPoint(390, 190)});
        click(topological, QPoint(170, 190));
        click(topological, QPoint(390, 190));
        drag(topological, QPoint(170, 190), QPoint(210, 230));
        auto topButtons = topological->findChildren<QPushButton *>();
        QVERIFY(topButtons.size() >= 2);
        QPushButton *sortButton = nullptr;
        for (auto *candidate : topButtons)
            if (candidate->text() == QStringLiteral("Sort graph")) sortButton = candidate;
        QVERIFY(sortButton);
        sortButton->click();
        QLabel *topResult = nullptr;
        for (auto *candidate : topological->findChildren<QLabel *>())
            if (candidate->text().startsWith(QStringLiteral("Order:"))) topResult = candidate;
        QVERIFY(topResult);
        QVERIFY(topResult->text().contains(QStringLiteral("1")));
        rightClick(topological, QPoint(600, 300));
        QVERIFY(topResult->text().contains(QStringLiteral("Graph changed")));
        QVERIFY(topological->findChildren<QLabel *>().size() >= 1);
        delete topological;

        QWidget *articulation = pageDescriptors()[8].create(nullptr);
        articulation->resize(800, 600);
        articulation->show();
        addGraphNodes(articulation, {QPoint(170, 190), QPoint(390, 190)});
        click(articulation, QPoint(170, 190));
        click(articulation, QPoint(390, 190));
        drag(articulation, QPoint(170, 190), QPoint(210, 230));
        auto artButtons = articulation->findChildren<QPushButton *>();
        QVERIFY(!artButtons.isEmpty());
        for (auto *candidate : artButtons)
            if (candidate->text().contains(QStringLiteral("Find articulation"))) candidate->click();
        QVERIFY(articulation->findChildren<QLabel *>().size() >= 2);
        QLabel *artResult = nullptr;
        for (auto *candidate : articulation->findChildren<QLabel *>())
            if (candidate->text().startsWith(QStringLiteral("Pass"))) artResult = candidate;
        QVERIFY(artResult);
        QTRY_VERIFY_WITH_TIMEOUT([&] {
            for (auto *candidate : articulation->findChildren<QLabel *>())
                if (candidate->text().startsWith(QStringLiteral("Complete"))) return true;
            return false;
        }(), 10000);
        rightClick(articulation, QPoint(610, 300));
        QVERIFY(artResult->isHidden());
        bool staleArtResult = false;
        for (auto *candidate : articulation->findChildren<QLabel *>())
            staleArtResult |= candidate->text().startsWith(QStringLiteral("Complete"));
        QVERIFY(!staleArtResult);
        captureIfRequested(articulation, QStringLiteral("articulation.png"));
        delete articulation;
    }

    void manuallyEditableGraphPagesDeleteEdgesAndIncidentNodes() {
        const QPoint a(170, 190), b(390, 190), c(280, 360);

        GraphUI::applyTheme(*qApp);
        VisComponents components;
        components.resize(800, 600);
        components.show();
        auto *componentDelete = button(&components, "componentsDeleteMode");
        auto *directed = components.findChild<QCheckBox *>("componentsDirected");
        QVERIFY(componentDelete && componentDelete->isCheckable() && directed);
        addGraphNodes(&components, {a, b, c});
        connectGraphNodes(&components, b, c);
        connectGraphNodes(&components, c, a);
        QCOMPARE(components.graph()->getNodes().size(), size_t(3));
        QCOMPARE(edgeCount(components.graph()), 2);

        // Starting a pending connection then toggling modes must cancel pair.
        click(&components, a);
        componentDelete->click();
        QVERIFY(componentDelete->isChecked());
        QCOMPARE(components.graph()->getNodes().size(), size_t(3));
        QCOMPARE(edgeCount(components.graph()), 2);
        rightClick(&components, QPoint(700, 450));
        QCOMPARE(components.graph()->getNodes().size(), size_t(3));
        drag(&components, c, c + QPoint(55, 45)); // delete mode must not move C
        QCOMPARE(components.graph()->getNodes().back().getCoord(), c);
        componentDelete->click();
        QVERIFY(!componentDelete->isChecked());
        click(&components, b);
        QCOMPARE(edgeCount(components.graph()), 2); // stale pending A did not create A-B

        componentDelete->click();
        click(&components, QPoint(335, 275)); // midpoint of B-C edge
        QCOMPARE(edgeCount(components.graph()), 1);
        QVERIFY(!hasEdge(components.graph(), 2, 3));
        QVERIFY(componentDelete->isChecked()); // mode persists after deletion
        click(&components, a);
        QCOMPARE(components.graph()->getNodes().size(), size_t(2));
        QCOMPARE(edgeCount(components.graph()), 0); // remaining C-A edge was incident
        rightClick(&components, QPoint(690, 440));
        QCOMPARE(components.graph()->getNodes().size(), size_t(2));
        componentDelete->click();
        rightClick(&components, QPoint(690, 440));
        QCOMPARE(components.graph()->getNodes().size(), size_t(3));
        QCOMPARE(components.graph()->getNodes().back().getIndex(), 4); // IDs never reused

        componentDelete->click();
        components.hide();
        components.show();
        QVERIFY(componentDelete->isChecked());

        // Directed toggle builds directed model; deleting an edge invalidates condensed results.
        VisComponents directedComponents;
        directedComponents.resize(800, 600);
        directedComponents.show();
        auto *directedDelete = button(&directedComponents, "componentsDeleteMode");
        auto *directedToggle = directedComponents.findChild<QCheckBox *>("componentsDirected");
        QVERIFY(directedDelete && directedToggle);
        directedToggle->setChecked(true);
        QVERIFY(directedComponents.graph()->graphKind() == GraphKind::Directed);
        addGraphNodes(&directedComponents, {a, b, c});
        connectGraphNodes(&directedComponents, a, b);
        connectGraphNodes(&directedComponents, b, c);
        connectGraphNodes(&directedComponents, c, a);
        auto *find = button(&directedComponents, "componentsFind");
        auto *condensed = button(&directedComponents, "componentsCondensed");
        QVERIFY(find && condensed);
        QTimer::singleShot(0, [] {
            if (QWidget *modal = QApplication::activeModalWidget()) modal->close();
        });
        find->click();
        QVERIFY(condensed->isEnabled());
        condensed->click();
        QVERIFY(condensed->text().contains(QStringLiteral("normal"), Qt::CaseInsensitive));
        directedDelete->click();
        click(&directedComponents, QPoint(280, 190));
        QCOMPARE(edgeCount(directedComponents.graph()), 2);
        QVERIFY(!hasEdge(directedComponents.graph(), 1, 2));
        QVERIFY(!condensed->isEnabled());
        QVERIFY(directedDelete->isChecked());
        captureIfRequested(&directedComponents, QStringLiteral("components-delete-mode.png"));

        VisTopologicalSort topological;
        topological.resize(800, 600);
        topological.show();
        auto *topDelete = button(&topological, "topologicalDeleteMode");
        QVERIFY(topDelete && topDelete->isCheckable());
        addGraphNodes(&topological, {a, b, c});
        connectGraphNodes(&topological, a, b);
        connectGraphNodes(&topological, b, c);
        connectGraphNodes(&topological, c, a);
        QCOMPARE(edgeCount(topological.graph()), 3);
        auto *sort = buttonWithText(&topological, QStringLiteral("Sort graph"));
        QVERIFY(sort);
        sort->click();
        QLabel *topResult = nullptr;
        for (auto *label : topological.findChildren<QLabel *>())
            if (label->text().contains(QStringLiteral("Cycle detected"))) topResult = label;
        QVERIFY(topResult);
        click(&topological, a);
        topDelete->click(); // cancels A as pending pair
        QVERIFY(topDelete->isChecked());
        rightClick(&topological, QPoint(690, 450));
        QCOMPARE(topological.graph()->getNodes().size(), size_t(3));
        topDelete->click();
        click(&topological, c); // absent A→C edge detects stale pending pair
        QCOMPARE(edgeCount(topological.graph()), 3);
        topDelete->click();
        drag(&topological, c, c + QPoint(50, 40));
        QCOMPARE(topological.graph()->findNode(3)->getCoord(), c);
        click(&topological, QPoint(280, 190));
        QCOMPARE(edgeCount(topological.graph()), 2);
        QVERIFY(!hasEdge(topological.graph(), 1, 2));
        sort->click();
        QVERIFY(topResult->text().startsWith(QStringLiteral("Order:")));
        click(&topological, a);
        QCOMPARE(topological.graph()->getNodes().size(), size_t(2));
        QCOMPARE(edgeCount(topological.graph()), 1);
        QVERIFY(topDelete->isChecked());
        topological.hide();
        topological.show();
        QVERIFY(topDelete->isChecked());
        QTest::qWait(25);
        captureIfRequested(&topological, QStringLiteral("topological-delete-mode.png"));

        VisArtBridge articulation;
        articulation.resize(800, 600);
        articulation.show();
        auto *artDelete = button(&articulation, "articulationDeleteMode");
        QVERIFY(artDelete && artDelete->isCheckable());
        addGraphNodes(&articulation, {a, b, c});
        connectGraphNodes(&articulation, a, b);
        connectGraphNodes(&articulation, b, c);
        connectGraphNodes(&articulation, c, a);
        const QPoint d(650, 360);
        addGraphNodes(&articulation, {d});
        click(&articulation, d);
        artDelete->click();
        QVERIFY(artDelete->isChecked());
        rightClick(&articulation, QPoint(700, 450));
        artDelete->click();
        click(&articulation, a); // a stale D selection would create D-A
        QCOMPARE(edgeCount(articulation.graph()), 3);
        auto *run = buttonWithText(&articulation, QStringLiteral("Find articulation points and bridges"));
        QVERIFY(run);
        run->click();
        auto *timer = articulation.findChild<QTimer *>();
        QVERIFY(timer && timer->isActive());
        artDelete->click();
        click(&articulation, QPoint(280, 190)); // delete one edge; cancel running result
        QCOMPARE(edgeCount(articulation.graph()), 2);
        QVERIFY(!hasEdge(articulation.graph(), 1, 2));
        QVERIFY(!timer->isActive());
        QVERIFY(artDelete->isChecked());
        QVERIFY(!run->isEnabled());
        run->click();
        QVERIFY(!timer->isActive());
        click(&articulation, a);
        QCOMPARE(articulation.graph()->getNodes().size(), size_t(3));
        QCOMPARE(edgeCount(articulation.graph()), 1);
        QVERIFY(!timer->isActive()); // structural edit stops animation and clears result
        bool staleArtResult = false;
        for (auto *label : articulation.findChildren<QLabel *>())
            staleArtResult |= label->isVisible() && (label->text().startsWith(QStringLiteral("Complete"))
                || label->text().startsWith(QStringLiteral("Pass")));
        QVERIFY(!staleArtResult);
        QVERIFY(artDelete->isChecked());
        articulation.hide();
        articulation.show();
        QVERIFY(artDelete->isChecked());
        captureIfRequested(&articulation, QStringLiteral("articulation-delete-mode.png"));
    }

    void flowDeleteModeRemovesIndependentDirectedEdgesAndRemapsEndpoints() {
        GraphUI::applyTheme(*qApp);
        VisFlow page;
        page.resize(1000, 700);
        page.show();
        auto *canvas = page.canvas();
        const QPoint a(170, 150), b(390, 150), c(610, 150);
        addFlowNode(canvas, a);
        addFlowNode(canvas, b);
        addFlowNode(canvas, c);
        const auto applyEdge = [&](QPoint from, QPoint to, const QString &capacityValue) {
            clickFlowNode(canvas, from);
            clickFlowNode(canvas, to);
            QVERIFY(canvas->edgeEditorVisible());
            auto *editor = canvas->edgeEditorWidget();
            auto *capacity = editor->findChild<QLineEdit *>("flowCapacity");
            auto *apply = button(editor, "flowApplyEdge");
            QVERIFY(capacity && apply);
            capacity->setText(capacityValue);
            apply->click();
            QVERIFY(!canvas->edgeEditorVisible());
        };
        applyEdge(a, b, QStringLiteral("7"));
        applyEdge(b, a, QStringLiteral("4"));
        applyEdge(b, c, QStringLiteral("5"));
        QCOMPARE(page.network()->getCapacity(1, 2), 7);
        QCOMPARE(page.network()->getCapacity(2, 1), 4);
        button(&page, "flowSource")->click();
        clickFlowNode(canvas, a);
        button(&page, "flowSink")->click();
        clickFlowNode(canvas, c);
        button(&page, "flowStart")->click();
        QVERIFY(!page.solver()->iterations().empty());
        const auto revision = page.network()->topologyRevision();
        auto *deleteMode = button(&page, "flowDeleteMode");
        QVERIFY(deleteMode && deleteMode->isCheckable());
        deleteMode->click();
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::Delete);
        click(canvas, QPoint(280, 139)); // 1→2 offset line
        QCOMPARE(page.network()->getCapacity(1, 2), 0);
        QCOMPARE(page.network()->getCapacity(2, 1), 4);
        QVERIFY(page.network()->topologyRevision() > revision);
        QCOMPARE(canvas->getIterationIndex(), -1);
        QVERIFY(page.solver()->iterations().empty());
        QVERIFY(!button(&page, "flowNext")->isEnabled());
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::Delete);
        drag(canvas, c, c + QPoint(45, 40));
        QCOMPARE(page.network()->getNodePositions()[2], QPointF(c));
        rightClick(canvas, QPoint(800, 500));
        QCOMPARE(page.network()->getNumNodes(), 3);
        click(canvas, QPoint(280, 161)); // opposite-direction 2→1 remains independently deletable
        QCOMPARE(page.network()->getCapacity(2, 1), 0);
        QCOMPARE(page.network()->getCapacity(2, 3), 5);
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::Delete);
        page.hide();
        page.show();
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::Delete);
        captureIfRequested(&page, QStringLiteral("flow-delete-mode.png"));
        button(&page, "flowEdit")->click();
        QCOMPARE(canvas->interactionMode(), NetworkWidget::InteractionMode::Edit);
        clickFlowNode(canvas, b); // pending pair canceled by deletion mode entry
        deleteMode->click();
        clickFlowNode(canvas, a);
        QCOMPARE(page.network()->getNumNodes(), 2);
        QCOMPARE(page.network()->getSource(), -1);
        QCOMPARE(page.network()->getSink(), 2);
        QCOMPARE(page.network()->getCapacity(1, 2), 5);
    }

    void mazeMstAndSpanningAlgorithmsStartAndPauseThroughUi() {
        VisMaze maze;
        maze.resize(1000, 700);
        maze.show();
        auto *mazeBfs = button(&maze, "mazeBfs");
        auto *mazeDfs = button(&maze, "mazeDfs");
        auto *mazeSpeed = maze.findChild<QSlider *>();
        auto *mazeStart = button(&maze, "mazeStart");
        auto *mazePause = buttonWithText(&maze, QStringLiteral("Pause"));
        QVERIFY(mazeBfs && mazeDfs && mazeSpeed && mazeStart && mazePause);
        QVERIFY(mazeBfs->isCheckable() && mazeBfs->isChecked());
        mazeDfs->click();
        QVERIFY(mazeDfs->isChecked());
        mazeSpeed->setValue(mazeSpeed->maximum());
        mazeStart->click();
        QVERIFY(mazePause->isEnabled());
        captureIfRequested(&maze, QStringLiteral("maze-running.png"));
        mazePause->click();
        QCOMPARE(mazePause->text(), QStringLiteral("Resume"));
        mazePause->click();
        QCOMPARE(mazePause->text(), QStringLiteral("Pause"));
        maze.hide();
        QCOMPARE(mazePause->text(), QStringLiteral("Resume"));

        VisMST mst;
        mst.resize(1400, 850);
        mst.show();
        auto *mstKruskal = button(&mst, "mstKruskal");
        auto *mstPrim = button(&mst, "mstPrim");
        auto *mstBoruvka = button(&mst, "mstBoruvka");
        auto *mstStart = button(&mst, "mstStart");
        auto *mstPause = buttonWithText(&mst, QStringLiteral("Pause"));
        auto *mstSpeed = mst.findChild<QSlider *>();
        QVERIFY(mstKruskal && mstPrim && mstBoruvka && mstStart && mstPause && mstSpeed);
        QVERIFY(mstKruskal->isChecked());
        mstPrim->click();
        QVERIFY(mstPrim->isChecked());
        mstSpeed->setValue(mstSpeed->maximum());
        mstStart->click();
        QVERIFY(mstPause->isEnabled());
        captureIfRequested(&mst, QStringLiteral("mst-running.png"));
        mst.hide();
        QCOMPARE(mstPause->text(), QStringLiteral("Resume"));
        mst.show();
        mstBoruvka->click();
        QVERIFY(mstBoruvka->isChecked());
        QVERIFY(!mstPause->isEnabled());
        mstStart->click();
        QVERIFY(mstPause->isEnabled());
        mst.hide();
        QCOMPARE(mstPause->text(), QStringLiteral("Resume"));

        // A second instance must initialize its own graph, independent of the shell page.
        VisSpanningTree secondSpanning;
        secondSpanning.resize(1100, 760);
        secondSpanning.show();
        auto *secondDfs = button(&secondSpanning, "spanningDfs");
        auto *secondBfs = button(&secondSpanning, "spanningBfs");
        auto *secondStart = button(&secondSpanning, "spanningStart");
        auto *secondPause = buttonWithText(&secondSpanning, QStringLiteral("Pause"));
        auto *secondSpeed = secondSpanning.findChild<QSlider *>();
        QVERIFY(secondDfs && secondBfs && secondStart && secondPause && secondSpeed);
        QVERIFY(secondStart->isEnabled());
        QVERIFY(!secondPause->isEnabled());
        secondDfs->click();
        QVERIFY(secondDfs->isChecked());
        secondSpeed->setValue(secondSpeed->maximum());
        secondStart->click();
        QVERIFY(secondPause->isEnabled());
        captureIfRequested(&secondSpanning, QStringLiteral("spanning-second-instance.png"));
        secondSpanning.hide();
        QCOMPARE(secondPause->text(), QStringLiteral("Resume"));
        secondSpanning.show();
        QCOMPARE(secondPause->text(), QStringLiteral("Resume"));
        secondPause->click();
        QCOMPARE(secondPause->text(), QStringLiteral("Pause"));
        secondSpanning.hide();
        QCOMPARE(secondPause->text(), QStringLiteral("Resume"));

    }

    void floodFillGeneratesEditableWallsAndCancelsPreviousAnimation() {
        GraphUI::applyTheme(*qApp);
        VisFloodFill page;
        page.resize(1000, 700);
        page.show();
        auto *generate = button(&page, "floodGenerateWalls");
        auto *clear = button(&page, "floodClearGrid");
        auto *timer = page.findChild<QTimer *>("floodFillTimer");
        auto *status = page.findChild<QLabel *>("floodStatus");
        QVERIFY(generate && clear && timer && status);

        const auto snapshot = [&page] {
            QImage image(page.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            page.render(&image);
            return image;
        };
        const auto findGridColor = [](const QImage &image, const QColor &color) {
            // Inspect visible grid cells below the toolbar, independently of model storage.
            for (int y = 116; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x)
                    if (image.pixelColor(x, y) == color) return QPoint(x, y);
            return QPoint(-1, -1);
        };
        QVERIFY(findGridColor(snapshot(), GraphUI::Text).x() < 0);
        generate->click();
        const QPoint wall = findGridColor(snapshot(), GraphUI::Text);
        QVERIFY(wall.x() >= 0);
        captureIfRequested(&page, QStringLiteral("flood-generated-walls.png"));

        // Generated walls support the same manual toggle/fill gestures as authored cells.
        click(&page, wall);
        QCOMPARE(snapshot().pixelColor(wall), GraphUI::Canvas);
        rightClick(&page, wall);
        QVERIFY(timer->isActive());
        generate->click();
        QVERIFY(!timer->isActive());
        QTest::qWait(50);
        QVERIFY(status->text().startsWith(QStringLiteral("Walls generated")));
        QVERIFY(findGridColor(snapshot(), GraphUI::Blue).x() < 0);

        const QPoint newWall = findGridColor(snapshot(), GraphUI::Text);
        QVERIFY(newWall.x() >= 0);
        click(&page, newWall);
        rightClick(&page, newWall);
        QTRY_COMPARE_WITH_TIMEOUT(status->text(), QStringLiteral("Flood Fill complete!"), 1500);
        QCOMPARE(snapshot().pixelColor(newWall), GraphUI::Blue);
        clear->click();
        QVERIFY(!timer->isActive());
        QVERIFY(findGridColor(snapshot(), GraphUI::Text).x() < 0);
        QVERIFY(findGridColor(snapshot(), GraphUI::Blue).x() < 0);
        click(&page, newWall);
        QCOMPARE(snapshot().pixelColor(newWall), GraphUI::Text);
    }

    void shellHonorsMinimumAndRequestedWindowSizes() {
        GraphVersePlatform shell;
        QCOMPARE(shell.minimumSize(), QSize(1120, 700));
        for (const QSize size : {QSize(1120, 700), QSize(1180, 780), QSize(1900, 1200)}) {
            shell.resize(size);
            shell.show();
            QCoreApplication::processEvents();
            QSize expected = size;
            // Cocoa constrains oversized windows to available screen height.
            if (QGuiApplication::platformName() == QStringLiteral("cocoa")) {
                const QMargins frame = shell.windowHandle()->frameMargins();
                const QSize available = shell.screen()->availableGeometry().size()
                    - QSize(frame.left() + frame.right(), frame.top() + frame.bottom());
                expected = size.boundedTo(available).expandedTo(shell.minimumSize());
            }
            QVERIFY(shell.size().width() >= expected.width());
            QVERIFY(shell.size().height() >= expected.height());
            auto *sidebar = shell.findChild<QWidget *>("sidebar");
            auto *content = shell.findChild<QWidget *>("contentArea");
            QVERIFY(sidebar && content);
            QVERIFY(sidebar->geometry().right() < content->geometry().left());
        }
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    AppIntegrationTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "app_integration_test.moc"
