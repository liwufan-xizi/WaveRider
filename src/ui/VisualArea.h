#pragma once

#include <QWidget>

class QStackedWidget;

namespace WaveRider {

class SpectrumWidget;
class LyricPanel;

enum class VisualMode {
    Spectrum,
    Lyrics,
};

/// Central visual area that hosts SpectrumWidget and LyricPanel.
/// Click to toggle between spectrum and lyrics views.
class VisualArea : public QWidget {
    Q_OBJECT
public:
    explicit VisualArea(QWidget* parent = nullptr);

    void setSpectrumWidget(SpectrumWidget* w);
    void setLyricPanel(LyricPanel* w);

    VisualMode mode() const { return m_mode; }

public slots:
    void setMode(VisualMode mode);
    void toggleMode();

signals:
    void modeChanged(VisualMode mode);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QStackedWidget* m_stack   = nullptr;
    SpectrumWidget* m_spectrum = nullptr;
    LyricPanel*     m_lyricPanel = nullptr;
    VisualMode      m_mode = VisualMode::Spectrum;
};

} // namespace WaveRider
