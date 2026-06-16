#include "ui/widgets/VolumeKnob.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

namespace WaveRider {

VolumeKnob::VolumeKnob(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(120, 28);
    setMouseTracking(true);
}

void VolumeKnob::setValue(float level)
{
    m_volume = qBound(0.0f, level, 1.0f);
    if (!m_muted) {
        m_savedVolume = m_volume;
    }
    update();
}

void VolumeKnob::toggleMute()
{
    m_muted = !m_muted;
    if (m_muted) {
        m_savedVolume = m_volume;
        emit valueChanged(0.0f);
    } else {
        m_volume = m_savedVolume;
        emit valueChanged(m_volume);
    }
    update();
}

// ── Painting ────────────────────────────────────────

void VolumeKnob::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int barHeight = 4;
    int barY = (height() - barHeight) / 2;
    int left  = 24;  // space for icon
    int right = width() - 4;

    // Background bar
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(80, 80, 80));
    p.drawRoundedRect(QRect(left, barY, right - left, barHeight), 2, 2);

    // Filled bar
    float displayVol = m_muted ? 0.0f : m_volume;
    int fillWidth = static_cast<int>(displayVol * (right - left));
    QColor fillColor = m_muted ? QColor(120, 120, 120) : QColor(26, 188, 156);
    p.setBrush(fillColor);
    p.drawRoundedRect(QRect(left, barY, fillWidth, barHeight), 2, 2);

    // Thumb
    int thumbX = left + fillWidth;
    p.setBrush(m_muted ? QColor(150, 150, 150) : QColor(255, 255, 255));
    p.drawEllipse(QPoint(thumbX, barY + barHeight / 2), 6, 6);

    // Speaker icon (simple triangle + arcs)
    p.setPen(QPen(QColor(200, 200, 200), 1.5));
    p.setBrush(Qt::NoBrush);

    // Speaker cone (triangle)
    int iconX = 6;
    int iconY = height() / 2;
    QPolygon cone;
    cone << QPoint(iconX + 6, iconY - 6)
         << QPoint(iconX + 13, iconY - 4)
         << QPoint(iconX + 13, iconY + 4)
         << QPoint(iconX + 6, iconY + 6);
    p.drawPolygon(cone);
    p.drawLine(iconX + 6, iconY - 6, iconX + 13, iconY - 4);
    p.drawLine(iconX + 6, iconY + 6, iconX + 13, iconY + 4);

    // Sound waves (if not muted)
    if (!m_muted && m_volume > 0.01f) {
        p.drawArc(iconX + 12, iconY - 8, 8, 16, -60 * 16, 120 * 16);
        if (m_volume > 0.3f) {
            p.drawArc(iconX + 15, iconY - 11, 12, 22, -70 * 16, 140 * 16);
        }
    }
    if (m_muted) {
        // X over speaker
        p.drawLine(iconX + 2, iconY - 7, iconX + 14, iconY + 7);
        p.drawLine(iconX + 2, iconY + 7, iconX + 14, iconY - 7);
    }
}

// ── Mouse interaction ───────────────────────────────

float VolumeKnob::volumeToX(float vol) const
{
    int left  = 24;
    int right = width() - 4;
    return left + vol * (right - left);
}

float VolumeKnob::xToVolume(int x) const
{
    int left  = 24;
    int right = width() - 4;
    float t = static_cast<float>(x - left) / (right - left);
    return qBound(0.0f, t, 1.0f);
}

void VolumeKnob::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Check if clicking on the speaker icon (toggle mute)
        if (event->pos().x() < 22) {
            toggleMute();
        } else {
            m_dragging = true;
            m_volume = xToVolume(event->pos().x());
            m_muted = false;
            m_savedVolume = m_volume;
            emit valueChanged(m_volume);
            update();
        }
    }
}

void VolumeKnob::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        m_volume = xToVolume(event->pos().x());
        m_muted = false;
        m_savedVolume = m_volume;
        emit valueChanged(m_volume);
        update();
    }
}

void VolumeKnob::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() / 120.0f * 0.05f;
    m_volume = qBound(0.0f, m_volume + delta, 1.0f);
    m_muted = false;
    m_savedVolume = m_volume;
    emit valueChanged(m_volume);
    update();
    event->accept();
}

} // namespace WaveRider
