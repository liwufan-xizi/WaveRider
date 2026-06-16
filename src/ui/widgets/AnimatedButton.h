#pragma once

#include <QWidget>

namespace WaveRider {

class AnimatedButton : public QWidget {
    Q_OBJECT
public:
    explicit AnimatedButton(QWidget* parent = nullptr);
};

} // namespace WaveRider
