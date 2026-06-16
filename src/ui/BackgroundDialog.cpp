#include "ui/BackgroundDialog.h"
#include "skin/BackgroundManager.h"
#include "skin/ThemeConfig.h"
#include "core/Constants.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFileDialog>
#include <QFileInfo>

namespace WaveRider {

// ── Mode display names ────────────────────────────────────
static const char* kModeLabels[] = { "Fill", "Fit", "Stretch", "Tile", "Center" };
static constexpr int kNumModes = 5;

// ═══════════════════════════════════════════════════════════
//  Construction
// ═══════════════════════════════════════════════════════════

BackgroundDialog::BackgroundDialog(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    setFixedSize(kWidth, kHeight);
    hide();
}

// ═══════════════════════════════════════════════════════════
//  Show / refresh
// ═══════════════════════════════════════════════════════════

void BackgroundDialog::refreshState()
{
    auto* bg = BackgroundManager::instance();
    m_hasBackground  = bg->hasCustomBackground();
    m_imagePath      = bg->currentBackgroundPath();
    m_mode           = bg->displayMode();
    m_blurRadius     = bg->blurRadius();
    m_overlayOpacity = bg->overlayOpacity();

    // Load preview
    if (m_hasBackground && !m_imagePath.isEmpty()) {
        m_previewPix = QPixmap(m_imagePath);
        if (m_previewPix.width() > 400)
            m_previewPix = m_previewPix.scaledToWidth(400, Qt::SmoothTransformation);
    } else {
        m_previewPix = QPixmap();
    }

    // Recompute layout rects (fixed positions, no dynamic content)
    m_titleRect  = QRect(14, 0, kWidth - 50, kHeaderH);
    m_closeRect  = QRect(kWidth - kHeaderH, 0, kHeaderH, kHeaderH);
    m_previewRect = QRect(14, kHeaderH + kPadV, kPreviewW, kPreviewH);
    m_infoRect   = QRect(m_previewRect.right() + 12, m_previewRect.top(),
                          kWidth - m_previewRect.right() - 26, kPreviewH);

    // Mode buttons
    m_modeRects.clear();
    int modeStartX = 14;
    int modeY      = m_previewRect.bottom() + kPadV + 12;
    int modeGap    = 6;
    int totalModeW = kBtnW * kNumModes + modeGap * (kNumModes - 1);
    // Center the mode row
    modeStartX = (kWidth - totalModeW) / 2;
    for (int i = 0; i < kNumModes; ++i) {
        int bx = modeStartX + i * (kBtnW + modeGap);
        m_modeRects.append(QRect(bx, modeY, kBtnW, kBtnH));
    }

    // Blur slider
    int sliderY = modeY + kBtnH + kPadV + 14;
    int sliderW = kWidth - 100;
    m_blurTrackRect  = QRect(60, sliderY + kHandleR - kTrackH/2, sliderW, kTrackH);
    m_blurHandleRect = QRect(0, 0, kHandleR * 2, kHandleR * 2);

    // Overlay slider
    int ovY = sliderY + kSliderH + 10;
    m_overlayTrackRect  = QRect(60, ovY + kHandleR - kTrackH/2, sliderW, kTrackH);
    m_overlayHandleRect = QRect(0, 0, kHandleR * 2, kHandleR * 2);

    // Action buttons
    int actY = ovY + kSliderH + 14;
    int actGap = 8;
    int totalActW = kActionW * 3 + actGap * 2;
    int actStartX = (kWidth - totalActW) / 2;
    m_browseRect = QRect(actStartX, actY, kActionW, kActionH);
    m_clearRect  = QRect(actStartX + kActionW + actGap, actY, kActionW, kActionH);
    m_noneRect   = QRect(actStartX + (kActionW + actGap) * 2, actY, kActionW, kActionH);
}

void BackgroundDialog::showDialog()
{
    refreshState();

    // Center on parent
    if (parentWidget()) {
        QPoint center = parentWidget()->geometry().center();
        move(center.x() - kWidth / 2, center.y() - kHeight / 2);
    }

    raise();
    show();
}

// ═══════════════════════════════════════════════════════════
//  Actions
// ═══════════════════════════════════════════════════════════

void BackgroundDialog::applyMode(int index)
{
    if (index < 0 || index >= kNumModes) return;
    auto mode = static_cast<BackgroundDisplayMode>(index);
    BackgroundManager::instance()->setDisplayMode(mode);
    m_mode = mode;
    emit backgroundChanged();
    update();
}

void BackgroundDialog::browseImage()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Select Background Image", QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All Files (*.*)");
    if (!path.isEmpty()) {
        BackgroundManager::instance()->setBackground(path);
        emit backgroundChanged();
        refreshState();
        update();
    }
}

void BackgroundDialog::clearImage()
{
    BackgroundManager::instance()->clearBackground();
    emit backgroundChanged();
    refreshState();
    update();
}

// ═══════════════════════════════════════════════════════════
//  Hit testing
// ═══════════════════════════════════════════════════════════

BackgroundDialog::HitZone BackgroundDialog::hitTest(const QPoint& pos) const
{
    if (m_closeRect.contains(pos))   return CloseBtn;
    if (m_browseRect.contains(pos))  return BrowseBtn;
    if (m_clearRect.contains(pos))   return ClearBtn;
    if (m_noneRect.contains(pos))    return NoneBtn;

    for (int i = 0; i < m_modeRects.size(); ++i)
        if (m_modeRects[i].contains(pos)) return ModeBtn;

    if (m_blurTrackRect.adjusted(-4, -8, 4, 8).contains(pos))
        return m_blurHandleRect.center() == pos ? BlurHandle : BlurTrack;
    if (m_overlayTrackRect.adjusted(-4, -8, 4, 8).contains(pos))
        return m_overlayHandleRect.center() == pos ? OverlayHandle : OverlayTrack;

    return None;
}

int BackgroundDialog::modeIndexAtPos(const QPoint& pos) const
{
    for (int i = 0; i < m_modeRects.size(); ++i)
        if (m_modeRects[i].contains(pos)) return i;
    return -1;
}

// ═══════════════════════════════════════════════════════════
//  Painting
// ═══════════════════════════════════════════════════════════

void BackgroundDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto& tc = ThemeConfig::instance()->colors();

    // ── Panel background ───────────────────────────
    QPainterPath panelPath;
    panelPath.addRoundedRect(rect(), kRadius, kRadius);
    p.fillPath(panelPath, QColor(14, 14, 26, 246));

    QPen borderPen(tc.accentDim);
    borderPen.setWidthF(1.0);
    p.setPen(borderPen);
    p.drawPath(panelPath);

    // ── Title ──────────────────────────────────────
    p.setPen(tc.textPrimary);
    QFont titleFont("Segoe UI", 13);
    titleFont.setWeight(QFont::DemiBold);
    p.setFont(titleFont);
    p.drawText(m_titleRect, Qt::AlignVCenter | Qt::AlignLeft, "Background Settings");

    // ── Close button ───────────────────────────────
    bool hoverClose = m_closeRect.contains(mapFromGlobal(QCursor::pos()));
    QColor closeCol = hoverClose ? tc.accent : tc.textSecondary;
    p.setPen(QPen(closeCol, 1.5));
    QFont closeFont("Segoe UI", 14);
    p.setFont(closeFont);
    p.drawText(m_closeRect, Qt::AlignCenter, QString::fromUtf8("×"));

    // ── Preview ────────────────────────────────────
    QPainterPath prevPath;
    prevPath.addRoundedRect(QRectF(m_previewRect), 4, 4);
    p.setClipPath(prevPath);
    if (!m_previewPix.isNull()) {
        // Scale to fill preview rect
        QPixmap scaled = m_previewPix.scaled(m_previewRect.size(),
            Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int cx = (scaled.width() - m_previewRect.width()) / 2;
        int cy = (scaled.height() - m_previewRect.height()) / 2;
        p.drawPixmap(m_previewRect.topLeft(), scaled, QRect(cx, cy,
            m_previewRect.width(), m_previewRect.height()));
    } else {
        p.fillPath(prevPath, QColor(20, 20, 36));
        p.setPen(tc.textSecondary);
        QFont phFont("Segoe UI", 10);
        p.setFont(phFont);
        p.drawText(m_previewRect, Qt::AlignCenter, "No Image");
    }
    p.setClipping(false);

    // Preview border
    p.setPen(QPen(QColor(40, 40, 60), 0.8));
    p.drawPath(prevPath);

    // ── Info text ──────────────────────────────────
    p.setPen(tc.textSecondary);
    QFont infoFont("Segoe UI", 9);
    p.setFont(infoFont);
    int iy = m_infoRect.top();
    int ilh = 18;
    if (m_hasBackground) {
        QFileInfo fi(m_imagePath);
        p.drawText(QRect(m_infoRect.left(), iy, m_infoRect.width(), ilh),
                   Qt::AlignVCenter | Qt::AlignLeft, "File: " + fi.fileName());
        iy += ilh;
        p.drawText(QRect(m_infoRect.left(), iy, m_infoRect.width(), ilh),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString("Size: %1×%2").arg(m_previewPix.width()).arg(m_previewPix.height()));
        iy += ilh;
        p.drawText(QRect(m_infoRect.left(), iy, m_infoRect.width(), ilh),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString("Mode: %1").arg(kModeLabels[static_cast<int>(m_mode)]));
    } else {
        p.drawText(QRect(m_infoRect.left(), iy, m_infoRect.width(), ilh),
                   Qt::AlignVCenter | Qt::AlignLeft, "No background set");
    }

    // ── Mode label ─────────────────────────────────
    int labelY = m_modeRects.first().top() - 18;
    p.setPen(tc.textSecondary);
    QFont labelFont("Segoe UI", 9);
    p.setFont(labelFont);
    p.drawText(QRect(14, labelY, 40, 16), Qt::AlignVCenter | Qt::AlignLeft, "Mode:");

    // ── Mode buttons ───────────────────────────────
    for (int i = 0; i < kNumModes; ++i) {
        const QRect& mr = m_modeRects[i];
        bool isActive  = (static_cast<int>(m_mode) == i);
        bool isHovered = (i == m_hoverModeIdx);

        QPainterPath btnPath;
        btnPath.addRoundedRect(QRectF(mr), 3, 3);

        QColor btnBg = isActive ? tc.accent :
                       isHovered ? QColor(30, 30, 50) :
                                   QColor(20, 20, 36);
        p.fillPath(btnPath, btnBg);

        QColor btnText = isActive ? QColor(10, 10, 20) : tc.textSecondary;
        p.setPen(btnText);
        QFont btnFont("Segoe UI", 10);
        if (isActive) btnFont.setWeight(QFont::DemiBold);
        p.setFont(btnFont);
        p.drawText(mr, Qt::AlignCenter, kModeLabels[i]);
    }

    // ── Blur slider ────────────────────────────────
    {
        p.setPen(tc.textSecondary);
        QFont sFont("Segoe UI", 9);
        p.setFont(sFont);
        p.drawText(QRect(14, m_blurTrackRect.top() - 8, 42, 16),
                   Qt::AlignVCenter | Qt::AlignLeft, "Blur:");

        QPainterPath trackPath;
        trackPath.addRoundedRect(QRectF(m_blurTrackRect), kTrackH/2, kTrackH/2);
        p.fillPath(trackPath, QColor(30, 30, 48));

        // Filled portion
        float blurFrac = m_blurRadius / 100.0f;
        int fillW = static_cast<int>(m_blurTrackRect.width() * blurFrac);
        if (fillW > 0) {
            QRectF fillRect(m_blurTrackRect.left(), m_blurTrackRect.top(),
                            fillW, kTrackH);
            QPainterPath fillPath;
            fillPath.addRoundedRect(fillRect, kTrackH/2, kTrackH/2);
            p.fillPath(fillPath, tc.accent);
        }

        // Handle
        int hx = m_blurTrackRect.left() + fillW;
        int hy = m_blurTrackRect.center().y();
        QPainterPath handlePath;
        handlePath.addEllipse(QPointF(hx, hy), kHandleR, kHandleR);
        p.fillPath(handlePath, tc.accent);
        p.setPen(QPen(tc.accent.lighter(140), 0.8));
        p.drawPath(handlePath);

        // Value
        p.setPen(tc.textSecondary);
        p.drawText(QRect(m_blurTrackRect.right() + 6, m_blurTrackRect.top() - 8, 36, 16),
                   Qt::AlignVCenter | Qt::AlignLeft, QString("%1").arg(m_blurRadius));
    }

    // ── Overlay slider ─────────────────────────────
    {
        p.setPen(tc.textSecondary);
        QFont sFont("Segoe UI", 9);
        p.setFont(sFont);
        p.drawText(QRect(14, m_overlayTrackRect.top() - 8, 42, 16),
                   Qt::AlignVCenter | Qt::AlignLeft, "Dim:");

        QPainterPath trackPath;
        trackPath.addRoundedRect(QRectF(m_overlayTrackRect), kTrackH/2, kTrackH/2);
        p.fillPath(trackPath, QColor(30, 30, 48));

        float ovFrac = m_overlayOpacity;
        int fillW = static_cast<int>(m_overlayTrackRect.width() * ovFrac);
        if (fillW > 0) {
            QRectF fillRect(m_overlayTrackRect.left(), m_overlayTrackRect.top(),
                            fillW, kTrackH);
            QPainterPath fillPath;
            fillPath.addRoundedRect(fillRect, kTrackH/2, kTrackH/2);
            p.fillPath(fillPath, tc.accent);
        }

        int hx = m_overlayTrackRect.left() + fillW;
        int hy = m_overlayTrackRect.center().y();
        QPainterPath handlePath;
        handlePath.addEllipse(QPointF(hx, hy), kHandleR, kHandleR);
        p.fillPath(handlePath, tc.accent);
        p.setPen(QPen(tc.accent.lighter(140), 0.8));
        p.drawPath(handlePath);

        // Value
        p.setPen(tc.textSecondary);
        p.drawText(QRect(m_overlayTrackRect.right() + 6, m_overlayTrackRect.top() - 8, 36, 16),
                   Qt::AlignVCenter | Qt::AlignLeft, QString("%1%").arg(static_cast<int>(m_overlayOpacity * 100)));
    }

    // ── Action buttons ─────────────────────────────
    auto drawActionBtn = [&](const QRect& r, const QString& text, bool isHover) {
        QPainterPath ap;
        ap.addRoundedRect(QRectF(r), 3, 3);
        p.fillPath(ap, isHover ? QColor(30, 30, 50) : QColor(20, 20, 36));
        p.setPen(QPen(QColor(40, 40, 60), 0.8));
        p.drawPath(ap);
        p.setPen(isHover ? tc.accent : tc.textSecondary);
        QFont af("Segoe UI", 10);
        p.setFont(af);
        p.drawText(r, Qt::AlignCenter, text);
    };

    bool hoverBrowse = m_browseRect.contains(mapFromGlobal(QCursor::pos()));
    bool hoverClear  = m_clearRect.contains(mapFromGlobal(QCursor::pos()));
    bool hoverNone   = m_noneRect.contains(mapFromGlobal(QCursor::pos()));

    drawActionBtn(m_browseRect, "Browse...", hoverBrowse);
    drawActionBtn(m_clearRect,  "Clear",     hoverClear);
    drawActionBtn(m_noneRect,   "None",      hoverNone);
}

// ═══════════════════════════════════════════════════════════
//  Mouse interaction
// ═══════════════════════════════════════════════════════════

void BackgroundDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    QPoint pos = event->pos();
    HitZone zone = hitTest(pos);

    switch (zone) {
    case CloseBtn:
        hide();
        emit dialogClosed();
        return;

    case ModeBtn: {
        int idx = modeIndexAtPos(pos);
        if (idx >= 0) applyMode(idx);
        return;
    }

    case BlurTrack:
    case BlurHandle: {
        m_activeZone = BlurTrack;
        // Set immediately at click position
        int trackX = m_blurTrackRect.left();
        int trackW = m_blurTrackRect.width();
        float frac = qBound(0.0f, static_cast<float>(pos.x() - trackX) / trackW, 1.0f);
        int newVal = qBound(0, static_cast<int>(frac * 100), 100);
        m_blurRadius = newVal;
        BackgroundManager::instance()->setBlurRadius(newVal);
        emit backgroundChanged();
        update();
        return;
    }

    case OverlayTrack:
    case OverlayHandle: {
        m_activeZone = OverlayTrack;
        int trackX = m_overlayTrackRect.left();
        int trackW = m_overlayTrackRect.width();
        float frac = qBound(0.0f, static_cast<float>(pos.x() - trackX) / trackW, 1.0f);
        float newVal = qBound(0.0f, frac, 1.0f);
        m_overlayOpacity = newVal;
        BackgroundManager::instance()->setOverlayOpacity(newVal);
        emit backgroundChanged();
        update();
        return;
    }

    case BrowseBtn:
        browseImage();
        return;

    case ClearBtn:
        clearImage();
        return;

    case NoneBtn:
        // "None" — same as clear but explicit
        BackgroundManager::instance()->clearBackground();
        emit backgroundChanged();
        refreshState();
        update();
        return;

    case None:
        break;
    }
}

void BackgroundDialog::mouseMoveEvent(QMouseEvent* event)
{
    // Handle slider drag
    if (m_activeZone == BlurTrack) {
        int trackX = m_blurTrackRect.left();
        int trackW = m_blurTrackRect.width();
        float frac = qBound(0.0f, static_cast<float>(event->pos().x() - trackX) / trackW, 1.0f);
        int newVal = qBound(0, static_cast<int>(frac * 100), 100);
        if (newVal != m_blurRadius) {
            m_blurRadius = newVal;
            BackgroundManager::instance()->setBlurRadius(newVal);
            emit backgroundChanged();
            update();
        }
        return;
    }
    if (m_activeZone == OverlayTrack) {
        int trackX = m_overlayTrackRect.left();
        int trackW = m_overlayTrackRect.width();
        float frac = qBound(0.0f, static_cast<float>(event->pos().x() - trackX) / trackW, 1.0f);
        if (qAbs(frac - m_overlayOpacity) > 0.005f) {
            m_overlayOpacity = frac;
            BackgroundManager::instance()->setOverlayOpacity(frac);
            emit backgroundChanged();
            update();
        }
        return;
    }

    // Update hover for mode buttons
    int oldHover = m_hoverModeIdx;
    m_hoverModeIdx = modeIndexAtPos(event->pos());
    if (m_hoverModeIdx != oldHover) update();
}

void BackgroundDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_activeZone = None;
    }
}

void BackgroundDialog::leaveEvent(QEvent*)
{
    if (m_hoverModeIdx != -1) {
        m_hoverModeIdx = -1;
        update();
    }
}

} // namespace WaveRider
