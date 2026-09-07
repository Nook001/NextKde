#include "trafficlightbutton.h"

#include "liquidglassdecoration.h"

#include <KDecoration3/DecoratedWindow>

#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace KOS
{

namespace
{
// macOS traffic light colours.
constexpr QRgb CloseTint = qRgb(0xff, 0x5f, 0x57);
constexpr QRgb MinimizeTint = qRgb(0xfe, 0xbc, 0x2e);
constexpr QRgb MaximizeTint = qRgb(0x28, 0xc8, 0x40);

// The glyph is drawn inside a square inset from the disc so the strokes stay
// clear of the rim, matching how small the real glyphs read.
constexpr qreal GlyphInsetRatio = 0.28;
}

// The size and position come from the decoration, which owns the row metrics
// and hands them to the button group.
TrafficLightButton::TrafficLightButton(KDecoration3::DecorationButtonType type,
                                       KDecoration3::Decoration *decoration,
                                       QObject *parent)
    : KDecoration3::DecorationButton(type, decoration, parent)
{
}

QColor TrafficLightButton::fillColor() const
{
    const auto *deco = qobject_cast<LiquidGlassDecoration *>(decoration());
    const bool windowActive = deco && deco->window() && deco->window()->isActive();

    if (!windowActive) {
        // Inactive windows grey every light out, as macOS does.
        return QColor(0x8c, 0x8c, 0x90, 0xcc);
    }

    QColor tint;
    switch (type()) {
    case KDecoration3::DecorationButtonType::Close:
        tint = QColor(CloseTint);
        break;
    case KDecoration3::DecorationButtonType::Minimize:
        tint = QColor(MinimizeTint);
        break;
    case KDecoration3::DecorationButtonType::Maximize:
        tint = QColor(MaximizeTint);
        break;
    default:
        tint = QColor(0x8c, 0x8c, 0x90);
        break;
    }

    if (!isEnabled()) {
        // The window forbids this action: keep the hue but drain it, so the
        // row still reads as three lights rather than losing a slot.
        tint.setAlphaF(0.35);
    } else if (isPressed()) {
        tint = tint.darker(118);
    }
    return tint;
}

void TrafficLightButton::paintGlyph(QPainter *painter, const QRectF &bounds) const
{
    const qreal inset = bounds.width() * GlyphInsetRatio;
    const QRectF glyph = bounds.adjusted(inset, inset, -inset, -inset);

    QPen pen(QColor(0, 0, 0, 0x8c));
    pen.setWidthF(bounds.width() * 0.11);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    switch (type()) {
    case KDecoration3::DecorationButtonType::Close:
        painter->drawLine(glyph.topLeft(), glyph.bottomRight());
        painter->drawLine(glyph.topRight(), glyph.bottomLeft());
        break;
    case KDecoration3::DecorationButtonType::Minimize:
        painter->drawLine(QPointF(glyph.left(), glyph.center().y()),
                          QPointF(glyph.right(), glyph.center().y()));
        break;
    case KDecoration3::DecorationButtonType::Maximize: {
        // Two opposed filled triangles, the macOS zoom glyph.
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 0x8c));
        QPainterPath topLeft;
        topLeft.moveTo(glyph.topLeft());
        topLeft.lineTo(glyph.left() + glyph.width() * 0.72, glyph.top());
        topLeft.lineTo(glyph.left(), glyph.top() + glyph.height() * 0.72);
        topLeft.closeSubpath();
        QPainterPath bottomRight;
        bottomRight.moveTo(glyph.bottomRight());
        bottomRight.lineTo(glyph.right() - glyph.width() * 0.72, glyph.bottom());
        bottomRight.lineTo(glyph.right(), glyph.bottom() - glyph.height() * 0.72);
        bottomRight.closeSubpath();
        painter->drawPath(topLeft);
        painter->drawPath(bottomRight);
        break;
    }
    default:
        break;
    }
}

void TrafficLightButton::paint(QPainter *painter, const QRectF &repaintArea)
{
    Q_UNUSED(repaintArea)

    if (!isVisible() || geometry().isEmpty()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF disc = geometry();

    painter->setPen(QPen(QColor(0, 0, 0, 0x29), 1.0 / std::max(1.0, decoration()->window()->scale())));
    painter->setBrush(fillColor());
    painter->drawEllipse(disc);

    // Real macOS reveals all three glyphs together as soon as the pointer is
    // over any one of them, so the row is queried, not just this button.
    const auto *deco = qobject_cast<LiquidGlassDecoration *>(decoration());
    if (isEnabled() && deco && deco->trafficLightsHovered()) {
        paintGlyph(painter, disc);
    }

    painter->restore();
}

} // namespace KOS
