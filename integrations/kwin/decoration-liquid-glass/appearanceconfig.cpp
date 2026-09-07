#include "appearanceconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace KOS
{

AppearanceConfig &AppearanceConfig::instance()
{
    static AppearanceConfig config;
    return config;
}

QString AppearanceConfig::configPath()
{
    // Quickshell.stateDir resolves to $XDG_STATE_HOME/quickshell/<shell id>,
    // and this shell's id is "kos".
    const QString stateDir = QStandardPaths::writableLocation(
        QStandardPaths::GenericStateLocation);
    return stateDir + QStringLiteral("/quickshell/kos/appearance/config.json");
}

AppearanceConfig::AppearanceConfig()
    : m_watcher(new QFileSystemWatcher(this))
    , m_path(configPath())
{
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this] {
        rearmWatch();
        reload();
    });
    // The shell saves by writing config.json.tmp and renaming it over the
    // target. That replaces the inode, so a file watch alone goes deaf after
    // the first save; watching the directory catches the rename.
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        rearmWatch();
        reload();
    });

    rearmWatch();
    reload();
}

void AppearanceConfig::rearmWatch()
{
    const QString dir = QFileInfo(m_path).absolutePath();
    if (!m_watcher->directories().contains(dir) && QDir(dir).exists()) {
        m_watcher->addPath(dir);
    }
    if (!m_watcher->files().contains(m_path) && QFile::exists(m_path)) {
        m_watcher->addPath(m_path);
    }
}

void AppearanceConfig::reload()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if (root.isEmpty()) {
        // A partially written file parses to nothing. Keeping the previous
        // values avoids a flash of unstyled decorations mid-save.
        return;
    }

    const qreal blur = qBound(0.0,
        root.value(QStringLiteral("globalBlurStrength")).toDouble(m_blurStrength), 1.0);
    const qreal liquid = qBound(0.0,
        root.value(QStringLiteral("globalLiquidStrength")).toDouble(m_liquidStrength), 1.0);

    if (qFuzzyCompare(blur, m_blurStrength) && qFuzzyCompare(liquid, m_liquidStrength)) {
        return;
    }

    m_blurStrength = blur;
    m_liquidStrength = liquid;
    Q_EMIT changed();
}

} // namespace KOS
