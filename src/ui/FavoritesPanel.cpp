#include "ui/FavoritesPanel.h"
#include "favorites/FavoritesManager.h"
#include "audio/AudioMetadata.h"
#include "core/SignalBus.h"
#include "skin/ThemeConfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QFileInfo>

namespace WaveRider {

FavoritesPanel::FavoritesPanel(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);

    m_anim = new QPropertyAnimation(this, "slideOffset", this);
    m_anim->setDuration(kAnimDurationMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished,
            this, &FavoritesPanel::onSlideOutFinished);

    setupUi();
    setupConnections();
    refreshFavorites();

    setSlideOffset(1.0f);
}

void FavoritesPanel::setupUi()
{
    auto* content = new QWidget(this);
    content->setObjectName("FavoritesContent");
    content->setFixedWidth(kPanelWidth);

    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    // ── Search bar ─────────────────────────────────────
    m_searchEdit = new QLineEdit(content);
    m_searchEdit->setPlaceholderText("Filter favorites...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setObjectName("FavoritesSearch");
    layout->addWidget(m_searchEdit);

    // ── Model ──────────────────────────────────────────
    m_model = new QStandardItemModel(this);

    // ── List view ──────────────────────────────────────
    m_listView = new QListView(content);
    m_listView->setModel(m_model);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setAlternatingRowColors(true);
    m_listView->setObjectName("FavoritesView");
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_listView, 1);

    // ── Connections ────────────────────────────────────
    connect(m_listView, &QListView::doubleClicked,
            this, &FavoritesPanel::onItemDoubleClicked);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &FavoritesPanel::onSearchTextChanged);
    connect(ThemeConfig::instance(), &ThemeConfig::themeColorsChanged,
            this, QOverload<>::of(&QWidget::update));

    m_listView->installEventFilter(this);
}

void FavoritesPanel::setupConnections()
{
    auto* bus = SignalBus::instance();
    connect(bus, &SignalBus::favoriteAdded,   this, &FavoritesPanel::refreshFavorites);
    connect(bus, &SignalBus::favoriteRemoved, this, &FavoritesPanel::refreshFavorites);
}

// ============================================================
// Data
// ============================================================
void FavoritesPanel::refreshFavorites()
{
    QString currentSearch = m_searchEdit ? m_searchEdit->text() : QString();

    m_model->removeRows(0, m_model->rowCount());

    QStringList paths = FavoritesManager::instance()->allFavorites();
    for (const QString& filePath : paths) {
        AudioMetadata meta = AudioMetadata::fromFile(filePath);

        QString title = meta.title;
        if (title.isEmpty()) {
            title = QFileInfo(filePath).completeBaseName();
        }

        auto* item = new QStandardItem(title);
        item->setData(filePath,                Qt::UserRole + 1);  // FilePathRole
        item->setData(meta.artist,             Qt::UserRole + 2);  // ArtistRole
        item->setData(AudioMetadata::formatDuration(meta.durationMs), Qt::UserRole + 3); // DurationRole
        item->setToolTip(QString("%1\n%2 — %3\n%4")
            .arg(title, meta.artist.isEmpty() ? "Unknown" : meta.artist,
                 meta.album, AudioMetadata::formatDuration(meta.durationMs)));
        item->setEditable(false);

        m_model->appendRow(item);
    }

    if (!currentSearch.isEmpty()) {
        onSearchTextChanged(currentSearch);
    }
}

// ============================================================
// Search filter
// ============================================================
void FavoritesPanel::onSearchTextChanged(const QString& text)
{
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem* item = m_model->item(i);
        if (!item) continue;
        QString title  = item->text();
        QString artist = item->data(Qt::UserRole + 2).toString();
        bool match = text.isEmpty()
            || title.contains(text, Qt::CaseInsensitive)
            || artist.contains(text, Qt::CaseInsensitive);
        m_listView->setRowHidden(i, !match);
    }
}

// ============================================================
// Double-click → play
// ============================================================
void FavoritesPanel::onItemDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QStandardItem* item = m_model->item(index.row());
    if (!item) return;
    QString filePath = item->data(Qt::UserRole + 1).toString();
    if (!filePath.isEmpty()) {
        emit trackDoubleClicked(filePath);
    }
}

// ============================================================
// Animation
// ============================================================
void FavoritesPanel::setSlideOffset(float offset)
{
    m_slideOffset = qBound(0.0f, offset, 1.0f);
    update();

    QWidget* content = findChild<QWidget*>("FavoritesContent");
    if (content) {
        int x = width() - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);
        content->move(x, 0);
        content->resize(kPanelWidth, height());
    }
}

void FavoritesPanel::slideIn()
{
    m_open = true;
    show();
    raise();
    QWidget* content = findChild<QWidget*>("FavoritesContent");
    if (content) content->resize(kPanelWidth, height());
    setSlideOffset(1.0f);

    m_anim->stop();
    m_anim->setStartValue(1.0f);
    m_anim->setEndValue(0.0f);
    m_anim->start();
}

void FavoritesPanel::slideOut()
{
    m_open = false;
    m_anim->stop();
    m_anim->setStartValue(m_slideOffset);
    m_anim->setEndValue(1.0f);
    m_anim->start();
}

void FavoritesPanel::onSlideOutFinished()
{
    if (!m_open && qFuzzyCompare(m_slideOffset, 1.0f)) {
        hide();
        emit slideOutFinished();
    }
}

// ============================================================
// Paint
// ============================================================
void FavoritesPanel::paintEvent(QPaintEvent*)
{
    if (qFuzzyCompare(m_slideOffset, 1.0f))
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    QColor surface = tc->surface();
    QColor border  = tc->surfaceBorder();
    float  overlayAlpha = tc->overlayAlpha();

    int w = width(), h = height();
    float backdropAlpha = (1.0f - m_slideOffset) * overlayAlpha;
    int contentX = w - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);

    // ── Backdrop ───────────────────────────────────────
    if (contentX > 0) {
        p.fillRect(0, 0, contentX, h, QColor(0, 0, 0, static_cast<int>(backdropAlpha * 255)));
    }

    // ── Content panel background ───────────────────────
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
void FavoritesPanel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    int contentX = width() - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);
    if (event->pos().x() < contentX) {
        slideOut();
        return;
    }
    QWidget::mousePressEvent(event);
}

void FavoritesPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QWidget* content = findChild<QWidget*>("FavoritesContent");
    if (content) {
        content->resize(kPanelWidth, height());
        int x = width() - kPanelWidth + static_cast<int>(m_slideOffset * kPanelWidth);
        content->move(x, 0);
    }
}

bool FavoritesPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_listView && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            QModelIndexList selected = m_listView->selectionModel()->selectedRows();
            for (const auto& idx : selected) {
                QStandardItem* item = m_model->item(idx.row());
                if (item) {
                    QString filePath = item->data(Qt::UserRole + 1).toString();
                    FavoritesManager::instance()->removeFavorite(filePath);
                }
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

} // namespace WaveRider
