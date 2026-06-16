#pragma once

#include <QWidget>
#include <QVector>

namespace WaveRider {

class EqualizerEffect;

/// Custom-painted 10-band equalizer control in Phigros style.
///
/// Visual design:
///   - 10 thin vertical tracks with circular drag handles
///   - Accent-color fill from neutral (0 dB) to current gain
///   - Frequency labels below, gain scale on the left
///   - Preset buttons above the slider bank
///   - Hover glow on the active band's track and handle
///
/// Interaction:
///   - Click on a track to set gain directly
///   - Drag the handle to adjust gain continuously
///   - Scroll wheel on a band for fine adjustment (±0.5 dB/step)
class EqualizerWidget : public QWidget {
    Q_OBJECT
public:
    explicit EqualizerWidget(QWidget* parent = nullptr);

    /// Bind to an equalizer effect for parameter sync.
    void setEffect(EqualizerEffect* effect);
    EqualizerEffect* effect() const { return m_effect; }

    /// Select a preset by index (0=Flat, 1=Rock, …).
    void loadPreset(int presetIndex);

    QSize sizeHint() const override;

signals:
    /// Emitted when the user drags a band.
    void bandChanged(int band, float gainDb);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    // ── Layout helpers ────────────────────────────────
    int  bandCount() const { return 10; }
    QRect bandRect(int band) const;       // full column for one band
    QRect trackRect(int band) const;      // the thin vertical track
    int   handleY(int band) const;        // y-position of handle center
    int   bandFromPos(const QPoint& pos) const;

    float yToGain(int y) const;           // pixel y → gain dB
    int   gainToY(float dB) const;        // gain dB → pixel y
    void  applyGain(int band, float dB);

    EqualizerEffect* m_effect = nullptr;

    // ── Interaction state ─────────────────────────────
    int  m_hoverBand = -1;        // which band the mouse is over
    int  m_dragBand  = -1;        // which band is being dragged
    bool m_dragging  = false;

    // ── Layout constants ──────────────────────────────
    static constexpr int kLeftMargin   = 30;
    static constexpr int kRightMargin  = 10;
    static constexpr int kTopMargin    = 34;
    static constexpr int kBottomMargin = 22;
    static constexpr int kTrackWidth   = 3;
    static constexpr int kHandleRadius = 5;
    static constexpr float kGainMin    = -15.0f;
    static constexpr float kGainMax    =  15.0f;
};

} // namespace WaveRider
