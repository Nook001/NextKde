#pragma once

#include <KDecoration3/Decoration>

// The QVariantList default argument below is instantiated by moc, which needs
// QVariant complete rather than merely forward-declared.
#include <QVariant>

namespace KDecoration3
{
class DecorationButtonGroup;
}

namespace KOS
{

// macOS-style window decoration for KOS.
//
// This is a compiled KDecoration3 plugin rather than a QML Aurorae theme on
// purpose: only a real decoration plugin can call setBorderRadius(), which is
// what makes KWin clip the *whole* window — including the client's own opaque
// content — to rounded corners. Aurorae does not link that symbol at all, so
// a QML theme can never round the bottom two corners; it can only draw a
// rounded rectangle inside its own title bar strip.
//
// The glass look is likewise delegated rather than painted: the title bar is
// published as the decoration's blur region, so the compositor's blur/glass
// effect frosts what is behind it. That keeps the effect scoped to decorated
// windows and leaves layer-shell surfaces (the KOS panels, the wallpaper)
// untouched.
class LiquidGlassDecoration final : public KDecoration3::Decoration
{
    Q_OBJECT

public:
    explicit LiquidGlassDecoration(QObject *parent = nullptr, const QVariantList &args = {});
    ~LiquidGlassDecoration() override;

    bool init() override;
    void paint(QPainter *painter, const QRectF &repaintArea) override;

    // True while the pointer is over any of the three lights: macOS reveals
    // all three glyphs together, so each button asks the row, not itself.
    bool trafficLightsHovered() const;

private:
    void createButtons();
    // Borders, border radius and outline are double-buffered through
    // DecorationState: borderTop() still reports the previous value right
    // after setBorders(). So the buffered properties are written here...
    void updateLayout();
    // ...and everything derived from the applied geometry is written from
    // here, which also runs on bordersChanged() once the compositor has taken
    // the new state.
    void updateDerivedGeometry();
    void updateBlurRegion();
    void updateTrafficLightPosition();

    bool isDark() const;
    QColor baseColor() const;
    QColor titleBarTint() const;
    QColor hairlineColor() const;
    QColor outlineColor() const;
    QColor captionColor() const;

    void paintTitleBar(QPainter *painter);
    void paintCaption(QPainter *painter);

    // Layout and paint dispatch only. The group explicitly does not accept
    // input events; the base Decoration routes those straight to the buttons,
    // which register themselves in DecorationButton's constructor.
    KDecoration3::DecorationButtonGroup *m_trafficLights = nullptr;
};

} // namespace KOS
