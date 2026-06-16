#pragma once

#include <QWidget>
#include <QVector>
#include "dsp/builtin/EqualizerEffect.h"

namespace WaveRider {

class DSPChain;
class CompressorEffect;
class ReverbEffect;
class EqualizerWidget;

/// DSP control panel with Phigros-style custom paint.
///
/// Contains:
///   - Master DSP toggle switch (thin outlined circle + power glyph)
///   - Effect tabs/selector (EQ | Compressor | Reverb)
///   - EqualizerWidget (10-band sliders + preset buttons)
///   - Future: Compressor/Reverb parameter controls
///
/// This is a floating/overlay panel, not embedded in the main layout.
/// It is toggled from the main menu or a control-bar button.
class DSPPanel : public QWidget {
    Q_OBJECT
public:
    explicit DSPPanel(DSPChain* chain, QWidget* parent = nullptr);

    void setDSPChain(DSPChain* chain);
    DSPChain* dspChain() const { return m_chain; }

    QSize sizeHint() const override;

signals:
    void panelClosed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    // ── Hit-test zones ────────────────────────────────
    enum HitZone {
        None,
        MasterToggle,
        PresetButton,
        CloseButton,
    };

    struct PresetBtn {
        QRect rect;
        EqualizerEffect::Preset preset;
        PresetBtn() = default;
        PresetBtn(QRect r, EqualizerEffect::Preset p) : rect(r), preset(p) {}
    };
    HitZone        hitTest(const QPoint& pos) const;
    const PresetBtn* presetAtPos(const QPoint& pos) const;

    // ── Internal helpers ──────────────────────────────
    void applyPreset(EqualizerEffect::Preset preset);
    void toggleMasterBypass();

    // ── DSP components ────────────────────────────────
    DSPChain*          m_chain    = nullptr;
    EqualizerEffect*   m_eqEffect = nullptr;
    EqualizerWidget*   m_eqWidget = nullptr;

    // ── Interaction state ─────────────────────────────
    HitZone            m_hoverZone = None;
    bool               m_masterOn  = true;

    // ── Layout rects (computed in paintEvent) ─────────
    QRect              m_masterToggleRect;
    QRect              m_closeRect;
    QVector<PresetBtn> m_presetBtns;

    // ── Layout constants ──────────────────────────────
    static constexpr int kWidth       = 480;
    static constexpr int kHeaderH     = 36;
    static constexpr int kPresetRowH  = 28;
    static constexpr int kEqHeight    = 190;
    static constexpr int kPadH        = 8;
    static constexpr int kRadius      = 6;
};

} // namespace WaveRider
