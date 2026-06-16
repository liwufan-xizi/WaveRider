#pragma once

#include <QWidget>
#include <QVector>
#include <QMap>

namespace WaveRider {

struct SkinInfo;  // fwd from skin/SkinManager.h

/// A compact swatch of key colors parsed from a skin's theme.qss @vars block.
/// Used to paint the preview card for each skin.
struct SkinPreviewColors {
    QString primary;
    QString bgMain;
    QString bgPanel;
    QString textPrimary;
    QString accent;
    QString surface;
    bool    valid = false;
};

/// Phigros-style skin selector overlay dialog.
///
/// Layout (360x280 fixed):
///   ┌──────────────────────────────────┐
///   │  Select Skin                  ×  │  title bar (36px)
///   ├──────────────────────────────────┤
///   │  ┌─────────┐  ┌─────────┐       │
///   │  │ ▓▓▓▓▓▓▓▓│  │ ▓▓▓▓▓▓▓▓│       │  skin cards
///   │  │ ▓▓▓▓▓▓▓▓│  │ ▓▓▓▓▓▓▓▓│       │  104x88 each
///   │  │ Phigros │  │DarkModern│       │  2 columns
///   │  └─────────┘  └─────────┘       │
///   └──────────────────────────────────┘
class SkinSelectorDialog : public QWidget {
    Q_OBJECT
public:
    explicit SkinSelectorDialog(QWidget* parent = nullptr);

    /// Populate cards from SkinManager and show at center of parent.
    void showDialog();

signals:
    void dialogClosed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    SkinPreviewColors parseSkinPreviewColors(const QString& qssPath) const;
    void               applySkin(int index);

    // ── Skin info ──────────────────────────────────
    QVector<SkinInfo>       m_skins;
    QVector<SkinPreviewColors> m_previews;
    int                     m_currentIndex = -1;

    // ── Interaction ────────────────────────────────
    int  m_hoverIndex = -1;
    int  m_pressedIndex = -1;

    // ── Layout rects (computed from constants) ─────
    QRect m_titleRect;
    QRect m_closeRect;
    QVector<QRect> m_cardRects;

    // ── Layout constants ───────────────────────────
    static constexpr int kWidth       = 360;
    static constexpr int kHeight      = 280;
    static constexpr int kHeaderH     = 36;
    static constexpr int kCardW       = 154;
    static constexpr int kCardH       = 88;
    static constexpr int kCardGap     = 12;
    static constexpr int kPadH        = 12;
    static constexpr int kTitlePadX   = 14;
    static constexpr int kRadius      = 6;
    static constexpr int kSwatchRows  = 2;
    static constexpr int kSwatchCols  = 4;
    static constexpr int kSwatchH     = 44;
    static constexpr int kLabelH      = 28;
};

} // namespace WaveRider
