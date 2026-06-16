#pragma once

#include <QObject>

namespace WaveRider {

class StyleSheetBuilder : public QObject {
    Q_OBJECT
public:
    explicit StyleSheetBuilder(QObject* parent = nullptr);
};

} // namespace WaveRider
