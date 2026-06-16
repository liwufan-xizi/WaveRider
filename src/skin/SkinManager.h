#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>

namespace WaveRider {

struct SkinInfo {
    QString name;           // "DarkModern"
    QString displayName;    // "Dark Modern"
    QString author;
    QString directoryPath;
    QString qssFilePath;
    bool    isBuiltIn = true;
};

/// Discovers, loads, and applies QSS skins at runtime.
class SkinManager : public QObject {
    Q_OBJECT
public:
    static SkinManager* instance();

    void scanSkinDirectories();
    void loadSkin(const QString& skinName);
    void reloadCurrentSkin();

    SkinInfo              currentSkin() const;
    QVector<SkinInfo>      availableSkins() const;
    QString                styleSheet() const;

signals:
    void skinChanged(const QString& skinName);
    void skinsDiscovered();

private:
    explicit SkinManager(QObject* parent = nullptr);
    SkinManager(const SkinManager&) = delete;
    SkinManager& operator=(const SkinManager&) = delete;

    QString resolveVariables(const QString& qss) const;
    void    applyStyleSheet(const QString& qss);

    QVector<SkinInfo> m_skins;
    SkinInfo          m_currentSkin;
    QMap<QString, QString> m_variables;
};

} // namespace WaveRider
