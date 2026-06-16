#pragma once

#include <QWidget>
#include "audio/AudioMetadata.h"

namespace WaveRider {

/// Compact track info bar (56px).
/// Custom-painted: album art placeholder + title/artist/info + bottom separator.
/// Colors from ThemeConfig singleton.
class TrackInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrackInfoWidget(QWidget* parent = nullptr);

    void setMetadata(const AudioMetadata& meta);
    void clear();
    void setScale(qreal scale);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    AudioMetadata m_meta;
    bool          m_hasTrack = false;
    qreal         m_scale    = 1.0;
};

} // namespace WaveRider
