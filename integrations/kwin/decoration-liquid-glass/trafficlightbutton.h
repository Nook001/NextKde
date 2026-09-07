#pragma once

#include <KDecoration3/DecorationButton>

namespace KOS
{

// A single macOS-style traffic light. KDecoration3::DecorationButton already
// wires the click action and the enabled state for its type in its own
// constructor, so this subclass must not connect those again: doing so would
// toggle maximization twice per click. It only supplies the round glass
// visuals.
class TrafficLightButton final : public KDecoration3::DecorationButton
{
    Q_OBJECT

public:
    TrafficLightButton(KDecoration3::DecorationButtonType type,
                       KDecoration3::Decoration *decoration,
                       QObject *parent = nullptr);

    void paint(QPainter *painter, const QRectF &repaintArea) override;

private:
    QColor fillColor() const;
    void paintGlyph(QPainter *painter, const QRectF &bounds) const;
};

} // namespace KOS
