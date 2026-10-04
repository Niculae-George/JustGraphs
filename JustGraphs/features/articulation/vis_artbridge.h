#ifndef VIS_ARTBRIDGE_H
#define VIS_ARTBRIDGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QHideEvent>
#include <QTime>
#include <QTimerEvent>
#include <memory>
#include "domain/graph.h"
#include "algorithms/articulation_bridge_solver.h"
#include "rendering/mutable_graph_renderer.h"
#include "ui/graph_edit_interaction.h"
#include "ui/playback_state.h"

// ─────────────────────────────────────────────────────────────
//  VisArtBridge
//  Visualizer: Articulation Points & Bridges (Tarjan DFS)
// ─────────────────────────────────────────────────────────────
class VisArtBridge : public QWidget {
	Q_OBJECT

public:
	explicit VisArtBridge(QWidget* parent = nullptr);
	~VisArtBridge();
	const Graph* graph() const { return m_graph.get(); }

protected:
	void paintEvent(QPaintEvent*) override;
	void resizeEvent(QResizeEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	void hideEvent(QHideEvent*) override;
	void showEvent(QShowEvent*) override;

private slots:
	void onRun();
	void onReset();
	void onAnimTick();

private:
	void setupUi();
	void resetState();
	void updateInfoPanel();

	// Graph interaction (same pattern as VisComponents)
	std::unique_ptr<Graph> m_graph;
	GraphEditInteraction m_interaction;

	// Algorithm result & animation
	ArtBridgeResult              m_result;
	std::vector<ArtBridgeAnimStep> m_steps;
	size_t                       m_animStep = 0;
	bool                         m_animating = false;

	// Animation state for rendering
	AnimationState               m_animState;

	// Renderer
	std::unique_ptr<MutableGraphRenderer> m_renderer;
	MutableRenderSettings        m_renderSettings;

	QTimer* m_timer;
	PlaybackState m_playback;
	int           m_tickInterval = 350; // ms per step — slow enough to follow

	// UI
	QPushButton* m_btnRun;
	QPushButton* m_btnReset;
	QPushButton* m_btnDeleteMode;
	QLabel* m_instrLabel;
	QLabel* m_resultLabel;
	QLabel* m_stepLabel;

	// Pulse animation for APs
	int   m_pulseTimer = 0;
	void  timerEvent(QTimerEvent*) override;
};

#endif // VIS_ARTBRIDGE_H
