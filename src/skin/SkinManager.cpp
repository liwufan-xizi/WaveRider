#include "skin/SkinManager.h"
#include "core/ConfigManager.h"
#include "core/SignalBus.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QDebug>

namespace WaveRider {

SkinManager::SkinManager(QObject* parent)
    : QObject(parent)
{
}

SkinManager* SkinManager::instance()
{
    static SkinManager mgr;
    return &mgr;
}

void SkinManager::scanSkinDirectories()
{
    m_skins.clear();

    // Scan built-in skins directory
    QString skinsRoot = QApplication::applicationDirPath() + "/../skins";
    QDir dir(skinsRoot);
    if (!dir.exists()) {
        // Development path: look relative to project root
        dir.setPath(QApplication::applicationDirPath() + "/../../skins");
    }
    if (!dir.exists()) {
        // Fallback: project source skins directory
        dir.setPath("skins");
    }

    for (const auto& entry : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString qssPath = entry.absoluteFilePath() + "/theme.qss";
        if (QFile::exists(qssPath)) {
            SkinInfo info;
            info.name          = entry.fileName();
            info.displayName   = entry.fileName();
            info.directoryPath = entry.absoluteFilePath();
            info.qssFilePath   = qssPath;
            info.isBuiltIn     = true;
            m_skins.append(info);
        }
    }

    emit skinsDiscovered();
}

void SkinManager::loadSkin(const QString& skinName)
{
    for (const auto& skin : m_skins) {
        if (skin.name == skinName) {
            QFile file(skin.qssFilePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString qss = QString::fromUtf8(file.readAll());
                file.close();

                // Resolve variables
                qss = resolveVariables(qss);

                // Apply to app
                applyStyleSheet(qss);

                m_currentSkin = skin;
                ConfigManager::instance()->setCurrentSkin(skinName);
                emit skinChanged(skinName);
                SignalBus::instance()->skinChanged(skinName);
                return;
            }
        }
    }
    qWarning() << "Skin not found:" << skinName;
}

void SkinManager::reloadCurrentSkin()
{
    if (!m_currentSkin.name.isEmpty()) {
        loadSkin(m_currentSkin.name);
    }
}

SkinInfo SkinManager::currentSkin() const
{
    return m_currentSkin;
}

QVector<SkinInfo> SkinManager::availableSkins() const
{
    return m_skins;
}

QString SkinManager::styleSheet() const
{
    return qApp->styleSheet();
}

QString SkinManager::resolveVariables(const QString& qss) const
{
    QString result = qss;
    QMap<QString, QString> vars;

    // Parse /* @vars ... */ comment block
    QRegularExpression varBlock("/\\*\\s*@vars(.*?)\\*/", QRegularExpression::DotMatchesEverythingOption);
    auto match = varBlock.match(result);
    if (match.hasMatch()) {
        QString block = match.captured(1);
        QRegularExpression varLine("\\$([a-zA-Z_]+)\\s*:\\s*([^;]+);");
        auto it = varLine.globalMatch(block);
        while (it.hasNext()) {
            auto m = it.next();
            vars["$" + m.captured(1)] = m.captured(2).trimmed();
        }
    }

    // Replace variables
    for (auto it = vars.begin(); it != vars.end(); ++it) {
        result.replace(it.key(), it.value());
    }

    return result;
}

void SkinManager::applyStyleSheet(const QString& qss)
{
    qApp->setStyleSheet(qss);
}

} // namespace WaveRider
