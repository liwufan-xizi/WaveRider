#pragma once

#include <QWidget>

namespace WaveRider {

class SeekSlider : public QWidget {
    Q_OBJECT
public:
    explicit SeekSlider(QWidget* parent = nullptr);
};

} // namespace WaveRider
