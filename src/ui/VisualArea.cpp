#include "ui/VisualArea.h"
#include "ui/SpectrumWidget.h"
#include "ui/LyricPanel.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QMouseEvent>

namespace WaveRider {

VisualArea::VisualArea(QWidget* parent)
    : QWidget(parent)
    , m_stack(new QStackedWidget(this))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stack);

    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VisualArea::setSpectrumWidget(SpectrumWidget* w)
{
    m_spectrum = w;
    m_stack->addWidget(w);
    m_stack->setCurrentWidget(w);
    m_mode = VisualMode::Spectrum;
}

void VisualArea::setLyricPanel(LyricPanel* w)
{
    m_lyricPanel = w;
    m_stack->addWidget(w);
}

void VisualArea::setMode(VisualMode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    m_stack->setCurrentWidget(mode == VisualMode::Spectrum
                              ? static_cast<QWidget*>(m_spectrum)
                              : static_cast<QWidget*>(m_lyricPanel));
    emit modeChanged(m_mode);
}

void VisualArea::toggleMode()
{
    setMode(m_mode == VisualMode::Spectrum
            ? VisualMode::Lyrics
            : VisualMode::Spectrum);
}

void VisualArea::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        toggleMode();
    }
    QWidget::mousePressEvent(event);
}

} // namespace WaveRider
