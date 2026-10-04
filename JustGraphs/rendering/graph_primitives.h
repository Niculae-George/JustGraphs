#ifndef GRAPH_PRIMITIVES_H
#define GRAPH_PRIMITIVES_H

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QString>

namespace GraphPrimitives {
inline void drawEdgeSegment(QPainter& painter, const QPointF& from, const QPointF& to,
                            const QColor& color, qreal width, Qt::PenStyle style = Qt::SolidLine) {
    painter.setPen(QPen(color, width, style, Qt::RoundCap));
    painter.drawLine(from, to);
}

inline void drawLabeledNode(QPainter& painter, const QRectF& bounds, const QColor& fill,
                            const QColor& border, const QString& label, const QColor& text,
                            int fontSize, qreal borderWidth = 1.0) {
    painter.setBrush(fill);
    painter.setPen(QPen(border, borderWidth));
    painter.drawEllipse(bounds);
    QFont font = painter.font();
    font.setPointSize(fontSize);
    painter.setFont(font);
    painter.setPen(text);
    painter.drawText(bounds, Qt::AlignCenter, label);
}
}

#endif
