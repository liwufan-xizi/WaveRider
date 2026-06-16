#pragma once

#include <QWidget>

namespace WaveRider {

/// Simple horizontal volume slider widget with mute toggle.
class VolumeKnob : public QWidget {
    Q_OBJECT
public:
    explicit VolumeKnob(QWidget* parent = nullptr);

    float value() const { return m_volume; }
    bool  isMuted() const { return m_muted; }

public slots:
    void setValue(float level);  // 0.0 – 1.0
    void toggleMute();

signals:
    void valueChanged(float level);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    float volumeToX(float vol) const;
    float xToVolume(int x) const;

    float m_volume = 0.8f;
    float m_savedVolume = 0.8f;
    bool  m_muted = false;
    bool  m_dragging = false;
};

} // namespace WaveRider
