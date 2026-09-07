#pragma once

#include <QObject>
#include <QString>

class QFileSystemWatcher;

namespace KOS
{

// Live view of the shell's glass settings.
//
// The Dock, the Bar and this decoration are meant to look like one material,
// so they must not each invent their own numbers. The shell owns the values in
// its appearance config; this reads the same file from inside the compositor
// and reports changes, so moving the slider in the shell restyles window
// decorations without a restart.
//
// Reading the file rather than talking to the shell is deliberate: the shell
// is an ordinary session process that can be restarted or absent, while KWin
// must be able to draw decorations regardless.
class AppearanceConfig final : public QObject
{
    Q_OBJECT

public:
    static AppearanceConfig &instance();

    // Both are the shell's normalized 0..1 sliders.
    qreal blurStrength() const
    {
        return m_blurStrength;
    }
    qreal liquidStrength() const
    {
        return m_liquidStrength;
    }

Q_SIGNALS:
    void changed();

private:
    AppearanceConfig();

    static QString configPath();
    void reload();
    void rearmWatch();

    QFileSystemWatcher *m_watcher = nullptr;
    QString m_path;
    // Defaults mirror AppearanceConfigService's, so a missing or unreadable
    // file looks the same as a freshly installed shell rather than fully
    // transparent.
    qreal m_blurStrength = 0.42;
    qreal m_liquidStrength = 1.0;
};

} // namespace KOS
