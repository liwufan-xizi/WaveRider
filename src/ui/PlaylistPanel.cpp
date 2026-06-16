#include "ui/PlaylistPanel.h"
#include "ui/PlaylistDelegate.h"
#include "playlist/PlaylistModel.h"
#include "skin/ThemeConfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QGraphicsOpacityEffect>

namespace WaveRider {

PlaylistPanel::PlaylistPanel(PlaylistModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    setMouseTracking(true);

    // ── Slide animation ────────────────────────────────
    m_anim = new QPropertyAnimation(this, "slideOffset", this);
    m_anim->setDuration(kAnimDurationMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished, this, &PlaylistPanel::onSlideOutFinished);

    setupUi();

    // Start hidden (off-screen)
    setSlideOffset(1.0f);
}

void PlaylistPanel::setupUi()
{
    // ── Content container (positioned by setSlideOffset) ──
    auto* content = new QWidget(this);
    content->setObjectName("PlaylistContent");
    content->setFixedWidth(kPanelWidth);

    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    // ── Header: search + clear ─────────────────────────
    auto* filterRow = new QHBoxLayout();
    filterRow->setSpacing(6);

    m_searchEdit = new QLineEdit(content);
    m_searchEdit->setPlaceholderText("Filter playlist...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setObjectName("PlaylistSearch");
    filterRow->addWidget(m_searchEdit, 1);

    m_clearBtn = new QPushButton("Clear", content);
    m_clearBtn->setObjectName("ClearButton");
    m_clearBtn->setToolTip("Clear playlist");
    m_clearBtn->setFixedWidth(50);
    filterRow->addWidget(m_clearBtn);

    layout->addLayout(filterRow);

    // ── Track list ─────────────────────────────────────
    m_listView = new QListView(content);
    m_listView->setModel(m_model);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setDragDropMode(QAbstractItemView::InternalMove);
    m_listView->setObjectName("PlaylistView");
    m_listView->setItemDelegate(new PlaylistDelegate(m_listView, this));
    layout->addWidget(m_listView, 1);

    // ── Connections ────────────────────────────────────
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            emit trackDoubleClicked(index.row());
        }
    });

    connect(m_clearBtn, &QPushButton::clicked, this, [this]() {
        emit clearRequested();
    });

    // Delete key removes selected tracks
    m_listView->installEventFilter(this);

    // Search filter
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            auto idx = m_model->index(i, 0);
            QString title  = m_model->data(idx, PlaylistModel::TitleRole).toString();
            QString artist = m_model->data(idx, PlaylistModel::ArtistRole).toString();
            bool match = text.isEmpty()
                || title.contains(text, Qt::CaseInsensitive)
                || artist.contains(text, Qt::CaseInsensitive);
            m_listView->setRowHidden(i, !match);
        }
    });

    // Connect ThemeConfig for repaint
    connect(ThemeConfig::instance(), &ThemeConfig::themeColorsChanged,
            this, QOverload<>::of(&QWidget::update));
}

// ============================================================
// Custom property setter for animation
// ============================================================
void PlaylistPanel::setSlideOffset(float offset)
{
    m_slideOffset = qBound(0.0f, offset, 1.0f);
    update();  // repaint backdrop

    // Reposition the content widget
    QWidget* content = findChild<QWidget*>("PlaylistContent");
    if (content) {
        int x = width() - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);
        content->move(x, 0);
        content->resize(kPanelWidth, height());
    }
}

// ============================================================
// Animation slots
// ============================================================
void PlaylistPanel::slideIn()
{
    m_open = true;
    show();
    raise();
    // Ensure children are sized before animating
    QWidget* content = findChild<QWidget*>("PlaylistContent");
    if (content) content->resize(kPanelWidth, height());
    setSlideOffset(1.0f);

    m_anim->stop();
    m_anim->setStartValue(1.0f);
    m_anim->setEndValue(0.0f);
    m_anim->start();
}

void PlaylistPanel::slideOut()
{
    m_open = false;
    m_anim->stop();
    m_anim->setStartValue(m_slideOffset);
    m_anim->setEndValue(1.0f);
    m_anim->start();
}

void PlaylistPanel::onSlideOutFinished()
{
    if (!m_open && qFuzzyCompare(m_slideOffset, 1.0f)) {
        hide();
        emit slideOutFinished();
    }
}

// ============================================================
// Paint — backdrop + content panel border
// ============================================================
void PlaylistPanel::paintEvent(QPaintEvent*)
{
    if (qFuzzyCompare(m_slideOffset, 1.0f))
        return;  // fully hidden, nothing to draw

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    QColor surface = tc->surface();
    QColor border  = tc->surfaceBorder();
    float  overlayAlpha = tc->overlayAlpha();

    int w = width(), h = height();

    // Backdrop opacity fades with the panel
    float backdropAlpha = (1.0f - m_slideOffset) * overlayAlpha;

    // ── Backdrop (area left of the content panel) ──────
    int contentX = w - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);
    if (contentX > 0) {
        p.fillRect(0, 0, contentX, h, QColor(0, 0, 0, static_cast<int>(backdropAlpha * 255)));
    }

    // ── Content panel background ───────────────────────
    // Draw slightly inset to leave room for the left border glow
    QRect panelRect(contentX, 0, w - contentX, h);
    p.setPen(QPen(border, 1.0));
    p.setBrush(surface);
    p.drawRect(panelRect);

    // ── Left edge accent line ──────────────────────────
    QColor accent = tc->accent();
    accent.setAlpha(60);
    p.setPen(QPen(accent, 1.5));
    p.drawLine(contentX, 0, contentX, h);
}

// ============================================================
// Events
// ============================================================
void PlaylistPanel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    // Click on the backdrop (to the left of the content panel) → dismiss
    int contentX = width() - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);
    if (event->pos().x() < contentX) {
        slideOut();
        return;
    }

    QWidget::mousePressEvent(event);
}

void PlaylistPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QWidget* content = findChild<QWidget*>("PlaylistContent");
    if (content) {
        content->resize(kPanelWidth, height());
        int x = width() - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);
        content->move(x, 0);
    }
}

bool PlaylistPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_listView && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            QModelIndexList selected = m_listView->selectionModel()->selectedRows();
            QList<int> rows;
            for (const auto& idx : selected)
                rows.append(idx.row());
            if (!rows.isEmpty()) {
                std::sort(rows.begin(), rows.end(), std::greater<int>());
                emit removeRequested(rows);
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

} // namespace WaveRider
