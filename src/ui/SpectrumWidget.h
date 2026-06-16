#pragma once

#include <QWidget>
#include <QTimer>
#include <windows.h>
#include "bass.h"

namespace WaveRider {

class AudioEngine;

/// Real-time frequency spectrum visualization.
/// Accent-colored bars with peak dots, glow, and smooth animation.
/// Colors from ThemeConfig singleton.
class SpectrumWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget* parent = nullptr);
    void setAudioEngine(AudioEngine* engine);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onTimerTick();

private:
    void fetchFftData();
    void applySmoothing();

    AudioEngine* m_engine = nullptr;
    QTimer*      m_timer  = nullptr;

    static constexpr int   kNumBars    = 64;
    static constexpr DWORD kFftFlags   = BASS_DATA_FFT1024;
    static constexpr int   kFftBins    = 512;
    static constexpr int   kBarGap     = 2;
    static constexpr float kDecaySpeed = 0.35f;

    float m_fftData[kNumBars]      = {};
    float m_smoothedData[kNumBars] = {};
    float m_peakData[kNumBars]     = {};  // falling peak dots
};

} // namespace WaveRider
