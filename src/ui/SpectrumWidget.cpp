#include "ui/SpectrumWidget.h"
#include "audio/AudioEngine.h"
#include "core/Constants.h"
#include "skin/ThemeConfig.h"

#include <QPainter>
#include <QPaintEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QLinearGradient>
#include <QtMath>
#include <algorithm>

namespace WaveRider {

SpectrumWidget::SpectrumWidget(QWidget* parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setMinimumHeight(100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);

    connect(m_timer, &QTimer::timeout, this, &SpectrumWidget::onTimerTick);

    // Repaint when theme colors change
    connect(ThemeConfig::instance(), &ThemeConfig::themeColorsChanged,
            this, [this]() { update(); });
}

void SpectrumWidget::setAudioEngine(AudioEngine* engine) { m_engine = engine; }

void SpectrumWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_timer->start(Config::FftTimerMs);
}

void SpectrumWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_timer->stop();
}

void SpectrumWidget::onTimerTick()
{
    fetchFftData();
    applySmoothing();
    update();
}

// ── FFT acquisition (unchanged logic, slightly cleaner) ─────
void SpectrumWidget::fetchFftData()
{
    if (!m_engine || !m_engine->streamHandle()) {
        std::fill_n(m_fftData, kNumBars, 0.0f);
        return;
    }

    float fft[kFftBins];
    DWORD result = BASS_ChannelGetData(
        m_engine->streamHandle(), fft,
        kFftFlags | BASS_DATA_FFT_REMOVEDC);

    if (result == static_cast<DWORD>(-1)) {
        std::fill_n(m_fftData, kNumBars, 0.0f);
        return;
    }

    const float binMax = static_cast<float>(kFftBins - 1);
    for (int bar = 0; bar < kNumBars; ++bar) {
        float t = static_cast<float>(bar) / (kNumBars - 1);
        float binStart = std::pow(binMax, std::sqrt(t) * 0.5f);
        float binEnd   = std::pow(binMax, std::sqrt((bar + 1.0f) / (kNumBars - 1)) * 0.5f);
        int startIdx = static_cast<int>(binStart);
        int endIdx   = qMin(static_cast<int>(binEnd) + 1, kFftBins - 1);
        if (endIdx <= startIdx) endIdx = startIdx + 1;

        float sum = 0.0f;
        for (int i = startIdx; i <= endIdx; ++i) sum += fft[i];
        m_fftData[bar] = sum / static_cast<float>(endIdx - startIdx + 1);
    }
}

// ── Smoothing + peak tracking ────────────────────────────────
void SpectrumWidget::applySmoothing()
{
    for (int i = 0; i < kNumBars; ++i) {
        if (m_fftData[i] >= m_smoothedData[i]) {
            m_smoothedData[i] = m_fftData[i];
            if (m_fftData[i] > m_peakData[i])
                m_peakData[i] = m_fftData[i];
        } else {
            m_smoothedData[i] = m_smoothedData[i] * (1.0f - kDecaySpeed)
                              + m_fftData[i] * kDecaySpeed;
            // Peak slowly falls
            m_peakData[i] = qMax(0.0f, m_peakData[i] - 0.006f);
        }
    }
}

// ── Paint ──────────────────────────────────────────────────
void SpectrumWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    const int w = width();
    const int h = height();

    // Transparent background — parent/window bg shows through
    p.fillRect(rect(), Qt::transparent);

    const int padding    = 12;
    const int barAreaW   = w - padding * 2;
    const float barWidth = static_cast<float>(barAreaW - kBarGap * (kNumBars - 1))
                         / static_cast<float>(kNumBars);
    const int maxBarH    = h - 24;
    const int baseY      = h - 10;

    if (barWidth < 1.0f) return;

    QColor accent    = tc->accent();
    QColor accentDim = tc->accentDim();

    for (int i = 0; i < kNumBars; ++i) {
        float amp = qMin(m_smoothedData[i], 1.0f);
        int barH  = qMax(2, static_cast<int>(amp * maxBarH));
        float barX = padding + i * (barWidth + kBarGap);
        float barY = static_cast<float>(baseY - barH);

        // ── Glow (behind bar) ──────────────────────────
        if (amp > 0.05f) {
            QRadialGradient glow(barX + barWidth * 0.5f, barY + barH * 0.5f, barH * 0.6f);
            QColor glowCol = accent;
            glowCol.setAlphaF(amp * 0.25f);
            glow.setColorAt(0.0, glowCol);
            glow.setColorAt(1.0, Qt::transparent);
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawRect(QRectF(barX - 2, barY - 2, barWidth + 4, barH + 4));
        }

        // ── Bar color: accent at bottom, lighter at top ──
        QLinearGradient grad(0, baseY, 0, barY);
        grad.setColorAt(0.0, accent);
        QColor topCol = accent.lighter(160);
        topCol.setAlpha(200);
        grad.setColorAt(1.0, topCol);

        QRectF barRect(barX, barY, barWidth, barH);
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawRoundedRect(barRect, 2.0, 2.0);

        // ── Peak dot ───────────────────────────────────
        if (m_peakData[i] > 0.02f) {
            float peakY = baseY - qMin(m_peakData[i], 1.0f) * maxBarH;
            p.setPen(QPen(accent, 2.0f));
            p.setBrush(accent.lighter(140));
            p.drawEllipse(QPointF(barX + barWidth * 0.5f, peakY), 2.5f, 2.5f);
        }
    }
}

} // namespace WaveRider
