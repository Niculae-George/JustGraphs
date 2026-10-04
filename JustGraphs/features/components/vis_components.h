#ifndef VIS_COMPONENTS_H
#define VIS_COMPONENTS_H
#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QHideEvent>
#include "domain/graph.h"
#include "rendering/component_graph_renderer.h"
#include "ui/graph_edit_interaction.h"
#include <memory>

class QLabel;

class VisComponents : public QWidget
{
    Q_OBJECT
public:
    explicit VisComponents(QWidget* parent = nullptr);
    ~VisComponents();
    const Graph* graph() const { return m_graph.get(); }
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void hideEvent(QHideEvent* event) override;
private slots:
    void onCheckBoxStateChanged(int state);
    void onFindComponents();
    void onToggleCondensedView();
private:
    std::unique_ptr<Graph> m_graph;
    GraphEditInteraction m_interaction;
    QPushButton* m_btnComponents;
    QPushButton* m_btnToggleCondensed;
    QPushButton* m_btnDeleteMode;
    QCheckBox* m_checkBox;
    QWidget* m_toolbar;
    QLabel* m_editHint;
    void recreateGraph(bool directed);
    void invalidateResults();
    void updateButtonText();
    ComponentResult m_compResult;
    CondensedResult m_condensedResult;
    
    // Renderer for component coloring (ISP: separated concern)
    std::unique_ptr<ComponentGraphRenderer> m_renderer;
};
#endif
