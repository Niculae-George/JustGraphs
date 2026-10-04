#ifndef PLAYBACK_STATE_H
#define PLAYBACK_STATE_H

// Shared lifecycle state for timer-driven visualizations. Timer scheduling and
// algorithm-specific frames remain owned by each page.
class PlaybackState {
public:
    enum class Phase { Idle, Running, Paused, Completed };
    Phase phase() const { return m_phase; }
    bool start() { if (m_phase == Phase::Running) return false; m_phase = Phase::Running; return true; }
    bool pause() { if (m_phase != Phase::Running) return false; m_phase = Phase::Paused; return true; }
    bool resume() { if (m_phase != Phase::Paused) return false; m_phase = Phase::Running; return true; }
    void complete() { if (m_phase == Phase::Running) m_phase = Phase::Completed; }
    void cancel() { m_phase = Phase::Idle; }
    bool active() const { return m_phase == Phase::Running || m_phase == Phase::Paused; }
private:
    Phase m_phase = Phase::Idle;
};

#endif
