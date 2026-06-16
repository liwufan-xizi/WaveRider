#include <QApplication>
#include <QMessageBox>
#include "core/Constants.h"
#include "core/ConfigManager.h"
#include "core/SignalBus.h"
#include "favorites/FavoritesManager.h"
#include "skin/SkinManager.h"
#include "skin/BackgroundManager.h"
#include "skin/ThemeConfig.h"
#include "ui/MainWindow.h"

using namespace WaveRider;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(Config::AppName);
    app.setOrganizationName(Config::OrgName);

    // Register meta-types for cross-thread signal/slot
    qRegisterMetaType<PlaybackState>("PlaybackState");
    qRegisterMetaType<PlayMode>("PlayMode");
    qRegisterMetaType<BackgroundDisplayMode>("BackgroundDisplayMode");

    // Initialize singletons
    ConfigManager::instance();
    SignalBus::instance();
    FavoritesManager::instance();

    // Initialize skin system and apply saved/default theme
    auto* skinMgr = SkinManager::instance();
    skinMgr->scanSkinDirectories();
    QString savedSkin = ConfigManager::instance()->currentSkin();
    skinMgr->loadSkin(savedSkin);

    // Initialize background manager (load saved background)
    // This also triggers ColorExtractor → ThemeConfig if a background is set
    BackgroundManager::instance()->loadSettings();

    // Initialize theme config (defaults applied if no background)
    ThemeConfig::instance();

    // Create and show main window
    MainWindow window;
    if (!window.initialize()) {
        return 1;
    }
    window.show();

    return app.exec();
}
