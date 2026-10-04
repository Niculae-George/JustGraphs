#ifndef VIS_FLOW_H
#define VIS_FLOW_H

#include <QWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QLineEdit>
#include <QHideEvent>
#include <memory>
#include <cstdint>
#include "domain/flownetwork.h"
#include "algorithms/flow_solver.h"
#include "ui/graph_edit_interaction.h"

class NetworkWidget;

class VisFlow : public QWidget {
    Q_OBJECT
public:
    explicit VisFlow(QWidget* parent = nullptr);
    ~VisFlow() override;
    const FlowNetwork* network() const { return m_network.get(); }
    const FlowSolver* solver() const { return m_solver.get(); }
    NetworkWidget* canvas() const { return m_networkWidget; }
protected:
    void hideEvent(QHideEvent* event) override;
private slots:
    void onStartAlgorithm();
    void onNextStep();
    void onEliminateNegCycles();
    void onReset();
    void onClear();
private:
    std::unique_ptr<FlowNetwork> m_network;
    std::unique_ptr<FlowSolver> m_solver;
    NetworkWidget* m_networkWidget;
    QPushButton* m_btnNextStep;
    QPushButton* m_btnStart;
    QPushButton* m_btnDelete;
    QLabel* m_statusLabel;
    void setupUI();
    void invalidateAlgorithm();
    void setStatus(const QString& text, bool error = false);
};

class NetworkWidget : public QWidget {
    Q_OBJECT
public:
    enum class InteractionMode { Edit, Delete, SetSource, SetSink, View };
    Q_ENUM(InteractionMode)
    explicit NetworkWidget(QWidget* parent = nullptr);
    void setNetwork(FlowNetwork* network);
    void setSolver(const FlowSolver* solver);
    void setMode(InteractionMode mode);
    InteractionMode interactionMode() const { return m_mode; }
    void setIterationIndex(int index);
    int getIterationIndex() const { return m_iterationIndex; }
    int selectedNode() const { return m_selectedNode; }
    int getSelectedNode() const { return m_selectedNode; }
    const FlowNetwork* network() const { return m_network; }
    void clearSelection() { setSelectedNode(-1); }
    bool edgeEditorVisible() const;
    QWidget* edgeEditorWidget() const { return m_edgeEditor; }
    void cancelTransientEditing();
public slots:
    void deleteSelectedNode();
signals:
    void networkEdited();
    void nodeSelectionChanged(int node);
    void statusMessage(const QString& message, bool error);
    void returnToEditMode();
    void interactionModeChanged(NetworkWidget::InteractionMode mode);
    void edgeRequested(int from, int to);
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private slots:
    void applyPendingEdge();
    void cancelPendingEdge();
private:
    FlowNetwork* m_network = nullptr;
    const FlowSolver* m_solver = nullptr;
    InteractionMode m_mode = InteractionMode::Edit;
    int m_iterationIndex = -1;
    int m_selectedNode = -1;
    QPoint m_tempMousePos;
    QPoint m_deletePressPos;
    bool m_deletePressed = false;
    GraphEditInteraction m_editInteraction;
    QWidget* m_edgeEditor = nullptr;
    QLineEdit* m_capacity = nullptr;
    QLineEdit* m_cost = nullptr;
    QLabel* m_edgeError = nullptr;
    QPushButton* m_applyEdge = nullptr;
    int m_pendingFrom = -1;
    int m_pendingTo = -1;
    static constexpr double NODE_RADIUS = 20.0;
    static constexpr double ARROW_SIZE = 12.0;
    void drawNode(QPainter& painter, int index, const QPointF& pos, bool isSource, bool isSink);
    void drawEdge(QPainter& painter, const QPointF& from, const QPointF& to,
                  int capacity, std::int64_t residual, int cost, bool isMinCut, bool isOnPath, bool isReverse = false);
    int getNodeAt(const QPoint& pos) const;
    void setSelectedNode(int node);
    void beginPendingEdge(int from, int to);
    void positionEdgeEditor();
    void setInlineError(const QString& message);
    QPointF clampedPosition(const QPointF& pos) const;
    void deleteAt(const QPoint& pos);
};

#endif
