#include "features/shortest_paths/vis_shortest_paths_page.h"

#include "ui/theme.h"

#include <QApplication>
#include <QButtonGroup>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResource>
#include <QSlider>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <unordered_set>

static void initializeMapResource() { Q_INIT_RESOURCE(maps); }

namespace {
constexpr qreal kMinZoom = 0.15;
constexpr qreal kMaxZoom = 36.0;
constexpr qreal kSelectRadius = 12.0;

QPointF eventPosition(const QMouseEvent* event) { return event->position(); }
}

LuxembourgMapCanvas::LuxembourgMapCanvas(const Luxembourg::Map* map, RoadRouteController* controller, QWidget* parent)
    : QWidget(parent), m_map(map), m_controller(controller) {
    setObjectName("mapCanvas");
    setMinimumSize(320, 240);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
    setAttribute(Qt::WA_OpaquePaintEvent);
    connect(m_controller, &RoadRouteController::endpointsChanged, this, &LuxembourgMapCanvas::endpointsChanged);
    connect(m_controller, &RoadRouteController::finished, this, &LuxembourgMapCanvas::routeFinished);
    connect(m_controller, &RoadRouteController::changed, this, [this] {
        const auto& visits = routeResult().settled;
        if (m_settledPoints.size() != qsizetype(visits.size()) || settledShown() == 0) {
            m_settledPoints.clear();
            m_settledPoints.reserve(qsizetype(visits.size()));
            for (int index : visits) m_settledPoints.append(m_map->nodes()[size_t(index)].position);
        }
        emit routeChanged();
        update();
    });
    rebuildRoadGeometry();
}

void LuxembourgMapCanvas::rebuildRoadGeometry() {
    m_roadSegments.clear();
    m_roadCacheDirty = true;
    if (!m_map) return;
    std::unordered_set<quint64> drawn;
    m_roadSegments.reserve(qsizetype(m_map->arcs().size()));
    for (const auto& arc : m_map->arcs()) {
        if (arc.from < 0 || arc.to < 0 || arc.from >= int(m_map->nodes().size()) || arc.to >= int(m_map->nodes().size()))
            continue;
        const quint64 key = (quint64(qMin(arc.from, arc.to)) << 32) | quint32(qMax(arc.from, arc.to));
        // Reciprocal arcs share road geometry; routing keeps both directions.
        if (drawn.insert(key).second)
            m_roadSegments.append(QLineF(m_map->nodes()[size_t(arc.from)].position,
                                       m_map->nodes()[size_t(arc.to)].position));
    }
}

QPointF LuxembourgMapCanvas::worldToScreen(QPointF world) const {
    return QPointF(width() * 0.5 + (world.x() - m_worldCenter.x()) * m_scale,
                   height() * 0.5 + (world.y() - m_worldCenter.y()) * m_scale);
}

QPointF LuxembourgMapCanvas::screenPosition(int index) const {
    if (!m_map || index < 0 || index >= int(m_map->nodes().size())) return QPointF(-1, -1);
    return worldToScreen(m_map->nodes()[size_t(index)].position);
}

QPointF LuxembourgMapCanvas::screenToWorld(QPointF screen) const {
    return QPointF(m_worldCenter.x() + (screen.x() - width() * 0.5) / m_scale,
                   m_worldCenter.y() + (screen.y() - height() * 0.5) / m_scale);
}

void LuxembourgMapCanvas::fitMap() {
    if (!m_map || m_map->nodes().empty()) return;
    QRectF bounds = m_map->bounds();
    if (!bounds.isValid() || bounds.isEmpty()) {
        bounds = QRectF(m_map->nodes().front().position, QSizeF(0, 0));
        for (const auto& node : m_map->nodes()) bounds |= QRectF(node.position, QSizeF(0, 0));
    }
    if (bounds.width() <= 0) bounds.setWidth(1);
    if (bounds.height() <= 0) bounds.setHeight(1);
    m_worldCenter = bounds.center();
    constexpr qreal margin = 24.0;
    const qreal sx = qMax<qreal>(1, width() - margin * 2) / bounds.width();
    const qreal sy = qMax<qreal>(1, height() - margin * 2) / bounds.height();
    m_fitScale = qMin(sx, sy);
    m_scale = m_fitScale;
    m_hasView = true;
    m_roadCacheDirty = true;
    m_userMovedView = false;
    update();
}

void LuxembourgMapCanvas::zoomAt(QPointF screen, qreal factor) {
    const QPointF anchor = screenToWorld(screen);
    const qreal relative = qBound(kMinZoom, m_scale * factor / m_fitScale, kMaxZoom);
    m_scale = m_fitScale * relative;
    m_worldCenter = QPointF(anchor.x() - (screen.x() - width() * 0.5) / m_scale,
                            anchor.y() - (screen.y() - height() * 0.5) / m_scale);
    m_userMovedView = true;
    m_roadCacheDirty = true;
    update();
}

void LuxembourgMapCanvas::zoomBy(qreal factor) {
    zoomAt(QPointF(width() * 0.5, height() * 0.5), factor);
}

void LuxembourgMapCanvas::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_roadCacheDirty = true;
    if (!m_hasView || !m_userMovedView) fitMap();
}

void LuxembourgMapCanvas::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!m_hasView) fitMap();
}

void LuxembourgMapCanvas::cancelInteraction() {
    m_pendingClick = m_dragging = m_dragged = false;
    setCursor(Qt::OpenHandCursor);
}

void LuxembourgMapCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), GraphUI::Canvas);
    if (!m_map || m_map->nodes().empty() || !m_hasView) return;

    const QPointF offset(width() * 0.5 - m_worldCenter.x() * m_scale,
                         height() * 0.5 - m_worldCenter.y() * m_scale);
    const qreal dpr = devicePixelRatioF();
    const QSize cacheSize(qCeil(width() * dpr), qCeil(height() * dpr));
    if (m_roadCacheDirty || m_roadCache.size() != cacheSize ||
        !qFuzzyCompare(m_roadCache.devicePixelRatioF(), dpr)) {
        m_roadCache = QPixmap(cacheSize);
        m_roadCache.setDevicePixelRatio(dpr);
        m_roadCache.fill(GraphUI::Canvas);
        QPainter roadsPainter(&m_roadCache);
        roadsPainter.setRenderHint(QPainter::Antialiasing);
        roadsPainter.translate(offset);
        roadsPainter.scale(m_scale, m_scale);
        QPen roads(GraphUI::Edge);
        roads.setWidthF(0.85 / m_scale);
        roads.setCapStyle(Qt::RoundCap);
        roadsPainter.setPen(roads);
        // Separate lines avoid rasterizing one enormous stroked path on Retina.
        roadsPainter.drawLines(m_roadSegments);
        m_roadCacheDirty = false;
    }
    painter.drawPixmap(0, 0, m_roadCache);
    painter.save();
    painter.setClipRect(rect());
    painter.translate(offset);
    painter.scale(m_scale, m_scale);

    if ((routing() || paused()) && settledShown() > 0) {
        QPen explored(QColor(GraphUI::Blue.red(), GraphUI::Blue.green(), GraphUI::Blue.blue(), 115));
        explored.setWidthF(3.6 / m_scale);
        explored.setCapStyle(Qt::RoundCap);
        painter.setPen(explored);
        painter.drawPoints(m_settledPoints.constData(), settledShown());
    }
    if (routeResult().found && !routing() && !paused() && routeResult().nodes.size() > 1) {
        QPainterPath route;
        const auto& first = m_map->nodes()[size_t(routeResult().nodes.front())].position;
        route.moveTo(first);
        for (size_t i = 1; i < routeResult().nodes.size(); ++i)
            route.lineTo(m_map->nodes()[size_t(routeResult().nodes[i])].position);
        QPen highlight(GraphUI::Blue);
        highlight.setWidthF(3.2 / m_scale);
        highlight.setCapStyle(Qt::RoundCap);
        highlight.setJoinStyle(Qt::RoundJoin);
        painter.setPen(highlight);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(route);
    }
    painter.restore();

    auto drawMarker = [this, &painter](int index, QColor fill, const QString& label, QPointF labelOffset) {
        if (index < 0 || index >= int(m_map->nodes().size())) return;
        const QPointF point = worldToScreen(m_map->nodes()[size_t(index)].position);
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(fill);
        painter.drawEllipse(point, 6.5, 6.5);
        painter.setPen(GraphUI::Text);
        QFont font = painter.font(); font.setPointSizeF(9); font.setBold(true); painter.setFont(font);
        QRectF labelRect(point + labelOffset, QSizeF(92, 22));
        painter.setPen(QPen(GraphUI::Border, 1));
        painter.setBrush(GraphUI::Surface);
        painter.drawRoundedRect(labelRect, 5, 5);
        painter.setPen(GraphUI::Text);
        painter.drawText(labelRect.adjusted(7, 0, -5, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
    };
    if (sourceIndex() == targetIndex() && sourceIndex() >= 0) {
        drawMarker(sourceIndex(), GraphUI::Success, "Start / target", QPointF(10, -29));
    } else {
        drawMarker(sourceIndex(), GraphUI::Success, "Start", QPointF(10, -29));
        drawMarker(targetIndex(), GraphUI::Warning, "Target", QPointF(10, 8));
    }
}

void LuxembourgMapCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return QWidget::mousePressEvent(event);
    m_pressPosition = eventPosition(event).toPoint();
    m_lastPosition = m_pressPosition;
    m_pendingClick = true;
    m_dragging = false;
    m_dragged = false;
    setCursor(Qt::ClosedHandCursor);
}

void LuxembourgMapCanvas::mouseMoveEvent(QMouseEvent* event) {
    const QPoint position = eventPosition(event).toPoint();
    if (!(event->buttons() & Qt::LeftButton) || !m_pendingClick) return QWidget::mouseMoveEvent(event);
    if (!m_dragging && (position - m_pressPosition).manhattanLength() >= QApplication::startDragDistance()) {
        m_dragging = true;
        m_dragged = true;
    }
    if (!m_dragging) return;
    const QPoint delta = position - m_lastPosition;
    m_worldCenter.rx() -= delta.x() / m_scale;
    m_worldCenter.ry() -= delta.y() / m_scale;
    m_roadCacheDirty = true;
    m_lastPosition = position;
    m_userMovedView = true;
    update();
}

void LuxembourgMapCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !m_pendingClick) return QWidget::mouseReleaseEvent(event);
    m_pendingClick = false;
    setCursor(Qt::OpenHandCursor);
    if (m_dragged) return;
    if (!m_map) return;
    const QPointF screen = eventPosition(event);
    const int candidate = m_map->nearestNode(screenToWorld(screen));
    if (candidate < 0 || candidate >= int(m_map->nodes().size())) return;
    if (QLineF(screen, worldToScreen(m_map->nodes()[size_t(candidate)].position)).length() > kSelectRadius) return;
    if (sourceIndex() < 0) setEndpoints(candidate, -1);
    else if (targetIndex() < 0) setEndpoints(sourceIndex(), candidate);
    else setEndpoints(sourceIndex(), candidate);
}

void LuxembourgMapCanvas::wheelEvent(QWheelEvent* event) {
    const qreal steps = !event->angleDelta().isNull()
        ? event->angleDelta().y() / 120.0
        : event->pixelDelta().y() / 100.0;
    if (qFuzzyIsNull(steps)) { event->ignore(); return; }
    zoomAt(event->position(), std::pow(1.18, steps));
    event->accept();
}

VisShortestPathsPage::VisShortestPathsPage(QWidget* parent)
    : QWidget(parent), m_map(std::make_unique<Luxembourg::Map>()) {
    setObjectName("shortestPathsPage");
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(12, 12, 12, 12);
    outer->setSpacing(8);

    auto* algorithmToolbar = GraphUI::toolbar(this);
    auto* algorithms = qobject_cast<QHBoxLayout*>(algorithmToolbar->layout());
    algorithms->setContentsMargins(10, 8, 10, 8);
    algorithms->setSpacing(7);
    const auto makeAlgorithmButton = [this, algorithms, algorithmToolbar](const QString& text, const QString& name, Luxembourg::Algorithm algorithm) {
        auto* button = GraphUI::button(text, algorithmToolbar);
        button->setObjectName(name);
        button->setCheckable(true);
        button->setProperty("role", QStringLiteral("algorithm"));
        algorithms->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, algorithm] { if (m_canvas) m_canvas->setAlgorithm(algorithm); });
        return button;
    };
    m_dijkstraButton = makeAlgorithmButton("Dijkstra", "mapDijkstra", Luxembourg::Algorithm::Dijkstra);
    m_aStarButton = makeAlgorithmButton("A*", "mapAStar", Luxembourg::Algorithm::AStar);
    m_biDijkstraButton = makeAlgorithmButton("Bi-Dijkstra", "mapBiDijkstra", Luxembourg::Algorithm::BidirectionalDijkstra);
    m_bellmanFordButton = makeAlgorithmButton("Bellman-Ford", "mapBellmanFord", Luxembourg::Algorithm::BellmanFord);
    auto* algorithmGroup = new QButtonGroup(this);
    algorithmGroup->setExclusive(true);
    for (auto* button : {m_dijkstraButton, m_aStarButton, m_biDijkstraButton, m_bellmanFordButton})
        algorithmGroup->addButton(button);
    m_floydWarshallButton = GraphUI::button("Floyd-Warshall", algorithmToolbar);
    m_floydWarshallButton->setObjectName("mapFloydWarshall");
    m_floydWarshallButton->setEnabled(false);
    m_floydWarshallButton->setToolTip("All-pairs costs require impractical memory for 42k road nodes.");
    algorithms->addWidget(m_floydWarshallButton);
    outer->addWidget(algorithmToolbar);

    auto* toolbar = GraphUI::toolbar(this);
    auto* controls = qobject_cast<QHBoxLayout*>(toolbar->layout());
    controls->setContentsMargins(10, 8, 10, 8);
    controls->setSpacing(7);
    m_startButton = GraphUI::button("Start", toolbar, true);
    m_startButton->setObjectName("mapStart");
    m_startButton->setEnabled(false);
    m_resetButton = GraphUI::button("Reset route", toolbar);
    m_resetButton->setObjectName("mapReset");
    m_pauseButton = GraphUI::button("Pause", toolbar);
    m_pauseButton->setObjectName("mapPause");
    m_pauseButton->setEnabled(false);
    m_speedSlider = new QSlider(Qt::Horizontal, toolbar);
    m_speedSlider->setObjectName("mapSpeed");
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(50);
    m_speedSlider->setMaximumWidth(140);
    m_speedSlider->setToolTip("Exploration speed");
    m_speedSlider->setAccessibleName("Exploration speed");
    m_fitButton = GraphUI::button("Fit map", toolbar);
    m_fitButton->setObjectName("mapFit");
    m_zoomOutButton = GraphUI::button("−", toolbar);
    m_zoomOutButton->setObjectName("mapZoomOut");
    m_zoomOutButton->setToolTip("Zoom out");
    m_zoomOutButton->setAccessibleName("Zoom out");
    m_zoomInButton = GraphUI::button("+", toolbar);
    m_zoomInButton->setObjectName("mapZoomIn");
    m_zoomInButton->setToolTip("Zoom in");
    m_zoomInButton->setAccessibleName("Zoom in");
    controls->addWidget(m_startButton);
    controls->addWidget(m_pauseButton);
    controls->addWidget(m_resetButton);
    controls->addWidget(new QLabel("Speed", toolbar));
    controls->addWidget(m_speedSlider);
    controls->addStretch(1);
    controls->addWidget(m_fitButton);
    controls->addWidget(m_zoomOutButton);
    controls->addWidget(m_zoomInButton);
    outer->addWidget(toolbar);

    m_statusLabel = GraphUI::status("Loading Luxembourg road graph…", this);
    m_statusLabel->setObjectName("mapStatus");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setMinimumHeight(34);
    outer->addWidget(m_statusLabel);

    initializeMapResource();
    QFile resource(":/maps/hartaLuxembourg.xml");
    QString error;
    if (resource.open(QIODevice::ReadOnly) && m_map->load(resource, error)) {
        m_controller = std::make_unique<RoadRouteController>(*m_map, m_router);
        m_canvas = new LuxembourgMapCanvas(m_map.get(), m_controller.get(), this);
        outer->addWidget(m_canvas, 1);
        connect(m_canvas, &LuxembourgMapCanvas::endpointsChanged, this, [this] {
            const bool ready = m_canvas->sourceIndex() >= 0 && m_canvas->targetIndex() >= 0;
            m_startButton->setEnabled(ready);
            updateStatus();
        });
        m_dijkstraButton->setChecked(true);
        m_canvas->setSpeed(m_speedSlider->value());
        connect(m_canvas, &LuxembourgMapCanvas::routeChanged, this, [this] {
            const bool busy = m_canvas->routing() || m_canvas->paused();
            m_pauseButton->setEnabled(busy);
            m_pauseButton->setText(m_canvas->paused() ? "Resume" : "Pause");
            updateStatus();
        });
        connect(m_canvas, &LuxembourgMapCanvas::routeFinished, this, &VisShortestPathsPage::updateStatus);
        connect(m_startButton, &QPushButton::clicked, m_canvas, &LuxembourgMapCanvas::startRoute);
        connect(m_pauseButton, &QPushButton::clicked, m_canvas, &LuxembourgMapCanvas::togglePause);
        connect(m_speedSlider, &QSlider::valueChanged, m_canvas, &LuxembourgMapCanvas::setSpeed);
        connect(m_resetButton, &QPushButton::clicked, m_canvas, &LuxembourgMapCanvas::clearRoute);
        connect(m_fitButton, &QPushButton::clicked, m_canvas, &LuxembourgMapCanvas::fitMap);
        connect(m_zoomInButton, &QPushButton::clicked, m_canvas, [this] { m_canvas->zoomBy(1.35); });
        connect(m_zoomOutButton, &QPushButton::clicked, m_canvas, [this] { m_canvas->zoomBy(1.0 / 1.35); });
        updateStatus();
    } else {
        if (error.isEmpty()) error = resource.errorString();
        m_statusLabel->setText(QString("Could not load Luxembourg map resource: %1").arg(error));
        m_statusLabel->setStyleSheet(QString("color: %1;").arg(GraphUI::Error.name()));
    }
}

void VisShortestPathsPage::updateStatus() {
    if (!m_map || !m_canvas || m_map->nodes().empty()) return;
    const QString graphInfo = QString("Luxembourg road network · %1 nodes · %2 directed arcs")
                                  .arg(m_map->nodes().size()).arg(m_map->arcs().size());
    QString interaction;
    const QString algorithm = [&] {
        switch (m_canvas->selectedAlgorithm()) {
        case Luxembourg::Algorithm::Dijkstra: return QString("Dijkstra");
        case Luxembourg::Algorithm::AStar: return QString("A*");
        case Luxembourg::Algorithm::BidirectionalDijkstra: return QString("Bidirectional Dijkstra");
        case Luxembourg::Algorithm::BellmanFord: return QString("Bellman-Ford");
        }
        return QString("Shortest path");
    }();
    if (m_canvas->paused()) {
        interaction = QString("%1 paused · %2 / %3 nodes visited")
            .arg(algorithm).arg(m_canvas->settledShown()).arg(m_canvas->routeResult().settled.size());
    } else if (m_canvas->routing()) {
        interaction = QString("%1 exploring… %2 / %3 nodes visited")
            .arg(algorithm)
            .arg(m_canvas->settledShown()).arg(m_canvas->routeResult().settled.size());
    } else if (m_canvas->routeLength() >= 0) {
        interaction = QString("%1 route cost: %2 · %3 nodes · Drag to pan · Scroll to zoom")
                          .arg(algorithm).arg(m_canvas->routeLength()).arg(m_canvas->routeResult().nodes.size());
    } else if (m_canvas->routeAttempted()) {
        interaction = "No directed route found · choose endpoints again or reset · Drag to pan · Scroll to zoom";
    } else if (m_canvas->sourceIndex() < 0) {
        interaction = "Click a road node to choose start · click again to choose target · Drag to pan · Scroll to zoom";
    } else if (m_canvas->targetIndex() < 0) {
        interaction = "Start selected · click another road node for target · Drag to pan · Scroll to zoom";
    } else {
        interaction = QString("Endpoints selected · Start %1 to calculate route · Drag to pan · Scroll to zoom").arg(algorithm);
    }
    m_statusLabel->setText(graphInfo + "\n" + interaction);
}

void VisShortestPathsPage::hideEvent(QHideEvent* event) {
    if (m_canvas) {
        m_canvas->pauseAnimation();
        m_canvas->cancelInteraction();
        updateStatus();
    }
    QWidget::hideEvent(event);
}
