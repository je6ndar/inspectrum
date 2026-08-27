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

#pragma once

#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QString>
#include "cursor.h"
#include "util.h"

/*
 * Horizontal cursor pair used to measure bandwidth on the spectrogram.
 *
 * Same interaction model as Cursors (which measures time), rotated 90
 * degrees: two draggable lines plus a draggable band in between.
 * Positions are viewport Y coordinates; PlotView converts them to Hz.
 */
class FreqCursors : public QObject
{
    Q_OBJECT

public:
    FreqCursors(QObject * parent);
    bool mouseEvent(QEvent::Type type, QMouseEvent *event);
    /* mouseEvent() split in two so PlotView can offer a grab on every
     * cursor line (time and frequency) before any whole-band drag */
    bool lineEvent(QEvent::Type type, QMouseEvent *event);
    bool bandEvent(QEvent::Type type, QMouseEvent *event);
    void leaveEvent();
    void paintFront(QPainter &painter, const QRect &rect);
    range_t<int> selection();
    void setSelection(range_t<int> selection);
    void setGridOpacity(int opacity) { gridAlpha = opacity; }
    /* read-outs drawn next to the cursors (top line, bottom line, band) */
    void setLabels(const QString &top, const QString &bottom,
                   const QString &band);

public slots:
    void cursorMoved();

signals:
    void cursorsMoved();

private:
    bool pointWithinDragRegion(QPoint point);

    Cursor *minCursor;   // upper line on screen -- higher frequency
    Cursor *maxCursor;   // lower line on screen -- lower frequency

    QPoint dragPos;                // keep track of dragging distance
    bool cursorOverride = false;   // used to record if cursor is overridden
    bool dragging = false;         // record if mouse is dragging
    int gridAlpha = 80;            // line opacity (0-255)

    QString topLabel;
    QString bottomLabel;
    QString bandLabel;
};
