#include "liquidglassdecoration.h"

#include <KPluginFactory>

// KWin's decoration bridge matches kwinrc's [org.kde.kdecoration3] library=
// key against this plugin's id, which is the installed file name:
// kos_liquid_glass. There is no separate theme= key for a compiled plugin.
K_PLUGIN_FACTORY_WITH_JSON(KosLiquidGlassDecorationFactory,
                           "metadata.json",
                           registerPlugin<KOS::LiquidGlassDecoration>();)

#include "main.moc"
