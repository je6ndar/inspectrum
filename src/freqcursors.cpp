/*
 *  Copyright (C) 2016, Mike Walters <mike@flomp.net>
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  This file is part of inspectrum.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QFontMetrics>
#include <QtGlobal>
#include <algorithm>
#include "freqcursors.h"

namespace {
/* colour of the bandwidth cursors -- amber, to tell them apart from the
 * white time cursors and the white/red tuner */
const QColor cursorColour(255, 200, 0);
}

FreqCursors::FreqCursors(QObject * parent) : QObject::QObject(parent)
{
    minCursor = new Cursor(Qt::Horizontal, Qt::SizeVerCursor, this);
    maxCursor = new Cursor(Qt::Horizontal, Qt::SizeVerCursor, this);
    connect(minCursor, &Cursor::posChanged, this, &FreqCursors::cursorMoved);
    connect(maxCursor, &Cursor::posChanged, this, &FreqCursors::cursorMoved);
}

void FreqCursors::cursorMoved()
{
    // Swap cursors if one has been dragged past the other
    if (minCursor->pos() > maxCursor->pos()) {
        std::swap(minCursor, maxCursor);
    }
    emit cursorsMoved();
}

bool FreqCursors::pointWithinDragRegion(QPoint point)
{
    int margin = 10;
    range_t<int> range = {minCursor->pos()+margin, maxCursor->pos()-margin};
    return range.contains(point.y());
}

bool FreqCursors::mouseEvent(QEvent::Type type, QMouseEvent *event)
{
    if (lineEvent(type, event))
        return true;
    return bandEvent(type, event);
}

bool FreqCursors::lineEvent(QEvent::Type type, QMouseEvent *event)
{
    if (minCursor->mouseEvent(type, event))
        return true;
    if (maxCursor->mouseEvent(type, event))
        return true;
    return false;
}

bool FreqCursors::bandEvent(QEvent::Type type, QMouseEvent *event)
{
    // If the mouse pointer is between the cursors, display a resize pointer
    if (pointWithinDragRegion(event->pos())) {
        if (!cursorOverride) {
            cursorOverride = true;
            QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
        }
    // Restore pointer otherwise
    } else {
        if (cursorOverride) {
            cursorOverride = false;
            QApplication::restoreOverrideCursor();
        }
    }
    // Start dragging on left mouse button press, if between the cursors
    if (type == QEvent::MouseButtonPress) {
        if (event->button() == Qt::LeftButton) {
            if (pointWithinDragRegion(event->pos())) {
                dragging = true;
                dragPos = event->pos();
                return true;
            }
        }
    // Update both cursor positions if we're dragging
    } else if (type == QEvent::MouseMove) {
        if (dragging) {
            int dy = event->pos().y() - dragPos.y();
            minCursor->setPos(minCursor->pos() + dy);
            maxCursor->setPos(maxCursor->pos() + dy);
            dragPos = event->pos();
            emit cursorsMoved();
        }
    // Stop dragging on left mouse button release
    } else if (type == QEvent::MouseButtonRelease) {
        if (event->button() == Qt::LeftButton && dragging) {
            dragging = false;
            return true;
        }
    }
    return false;
}

void FreqCursors::leaveEvent()
{
    minCursor->leaveEvent();
    maxCursor->leaveEvent();

    if (cursorOverride) {
        cursorOverride = false;
        QApplication::restoreOverrideCursor();
    }
}

void FreqCursors::setLabels(const QString &top, const QString &bottom,
                            const QString &band)
{
    topLabel = top;
    bottomLabel = bottom;
    bandLabel = band;
}

void FreqCursors::paintFront(QPainter &painter, const QRect &rect)
{
    if (gridAlpha <= 0)
        return; /* fully transparent - draw nothing */

    int top = minCursor->pos();
    int bot = maxCursor->pos();

    /* nothing to draw if the band is completely off the visible area */
    if (bot < rect.top() && top < rect.top())
        return;
    if (top > rect.bottom() && bot > rect.bottom())
        return;

    painter.save();
    painter.setClipRect(rect);

    /* scale fill opacity with cursor opacity (base 50 at full opacity) */
    int fillAlpha = gridAlpha * 50 / 255;
    QColor fill = cursorColour;
    fill.setAlpha(fillAlpha);
    painter.fillRect(QRect(rect.left(), top, rect.width(), bot - top),
                     QBrush(fill));

    QColor line = cursorColour;
    line.setAlpha(gridAlpha);
    painter.setPen(QPen(line, 1, Qt::SolidLine));
    painter.drawLine(rect.left(), top, rect.right(), top);
    painter.drawLine(rect.left(), bot, rect.right(), bot);

    /* read-outs: frequency of each line, bandwidth in the middle */
    if (!topLabel.isEmpty() || !bandLabel.isEmpty()) {
        QColor text = cursorColour;
        text.setAlpha(std::max(gridAlpha, 160));
        painter.setPen(QPen(text, 1, Qt::SolidLine));

        QFontMetrics fm(painter.font());
        int x = rect.right() - 8;
        int lineHeight = fm.height();

        auto drawRight = [&](const QString &s, int y) {
            if (s.isEmpty() || y < rect.top() || y > rect.bottom())
                return;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
            int w = fm.horizontalAdvance(s);
#else
            int w = fm.width(s);
#endif
            painter.drawText(x - w, y, s);
        };

        drawRight(topLabel, top - 4);
        drawRight(bottomLabel, bot + lineHeight);
        if (bot - top >= 2 * lineHeight)
            drawRight(bandLabel, (top + bot + lineHeight) / 2);
    }

    painter.restore();
}

range_t<int> FreqCursors::selection()
{
    return {minCursor->pos(), maxCursor->pos()};
}

void FreqCursors::setSelection(range_t<int> selection)
{
    minCursor->setPos(selection.minimum);
    maxCursor->setPos(selection.maximum);
}
