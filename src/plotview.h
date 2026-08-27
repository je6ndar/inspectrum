/*
 *  Copyright (C) 2015-2016, Mike Walters <mike@flomp.net>
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

#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonObject>
#include <QPaintEvent>

#include "cursors.h"
#include "freqcursors.h"
#include "inputsource.h"
#include "plot.h"
#include "samplesource.h"
#include "spectrogramplot.h"
#include "thresholdplot.h"
#include "traceplot.h"

struct DerivedPlotInfo {
	int parentIndex;
	QString typeName;
};

enum DemodMode { DemodAmplitude = 0, DemodFrequency = 1, DemodPhase = 2 };

struct DetectResult {
	double rate = 0;
	int transitions = 0;
	QString status;
};

class PlotView : public QGraphicsView, Subscriber
{
    Q_OBJECT

public:
    PlotView(InputSource *input);
    ~PlotView();
    void setSampleRate(double rate);

signals:
    void timeSelectionChanged(float time, float offset);
    void freqSelectionChanged(double lowHz, double highHz);
    void segmentsChanged(int segments);
    void tunerChanged(double centreHz, double bandwidthHz);
    void renderTimeChanged(int ms);
    void viewPositionChanged(double timeSec, double freqHz);
    void zoomIn();
    void zoomOut();

public slots:
    void cursorsMoved();
    void enableCursors(bool enabled);
    void lockCursors(bool locked);
    void resetCursorState();
    void setCursorGridOpacity(int opacity);
    void freqCursorsMoved();
    void enableFreqCursors(bool enabled);
    void setFreqSelection(double lowHz, double highHz);
    range_t<double> getSelectedFreqs() { return selectedFreqs; }
    void enableScales(bool enabled);
    void enableAnnotations(bool enabled);
    void enableAnnotationCommentsTooltips(bool enabled);
    void invalidateEvent() override;
    void repaint();
    void setCursorSegments(int segments);
    void setSegmentsOnly(int segments);
    void setSymbolRate(double rate);
    void setPeriod(double seconds);
    void setOffset(double seconds);
    void restoreSessionPlots(const QJsonArray &plotsArray);
    void refreshThresholdPlots();
    void invalidateThresholdData();
    void setSelectedSamples(range_t<size_t> samples);
    void setScrollPosition(int hValue, int vValue);
    range_t<size_t> getSelectedSamples() { return selectedSamples; }
    QJsonArray getDerivedPlotsState();
    int getTunerCentre();
    int getTunerDeviation();
    void setTunerPosition(int centre, int deviation);
    void setTunerCentreHz(double hz);
    void setTunerBandwidthHz(double hz);
    void setFFTAndZoom(int fftSize, int zoomLevel);
    void setZeroPad(int level);
    void setZoomY(int level);
    void setCropToTuner(bool enabled);
    void setTunerVisible(bool visible);
    void jumpToBookmark(double timeSec, double freqHz);
    void jumpToTime(double timeSec);
    void jumpToFreq(double freqHz);
    DetectResult autoDetectSymbolRate(DemodMode mode);
    void setLsbFirst(bool lsb);
    void setPowerMin(int power);
    void setPowerMax(int power);
    void setAveraging(int count);
    void setOverlap(int index);
    void setWindowType(int index);
    void setKaiserBeta(double beta);
    void setColormapType(int index);
    void setAveragingMode(int index);
    void setAveragingAlpha(double alpha);
    void setNoiseFloorMethod(int index);
    void setNoiseFloorPercentile(int pct);
    void setTFRMode(int index);
    void setReassignThreshold(double dB);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent * event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent * event) override;
    void scrollContentsBy(int dx, int dy) override;
    bool viewportEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Cursors cursors;
    FreqCursors freqCursors;
    SampleSource<std::complex<float>> *mainSampleSource = nullptr;
    SpectrogramPlot *spectrogramPlot = nullptr;
    std::vector<std::unique_ptr<Plot>> plots;
    std::vector<DerivedPlotInfo> derivedPlotInfos;
    range_t<size_t> viewRange = {0, 0};
    range_t<size_t> selectedSamples = {0, 0};
    int zoomPos = 0;
    size_t zoomSample = 0;

    int fftSize = 1024;
    int zoomLevel = 1;
    int powerMin = -100;
    int powerMax = 0;
    bool cursorsEnabled = false;
    bool cursorsLocked = false;
    bool hadCursors = false;
    range_t<size_t> savedSelectedSamples = {0, 0};
    bool freqCursorsEnabled = false;
    /* shift+click / shift+drag cursor placement */
    bool cursorPlaceDrag = false;
    bool cursorPlaceMoved = false;
    QPoint cursorPlaceStart;
    bool hadFreqCursors = false;
    /* bandwidth cursor positions, kept in Hz so they stay on the signal
     * across scrolling, FFT size changes and Y zoom */
    range_t<double> selectedFreqs = {0, 0};
    range_t<double> savedSelectedFreqs = {0, 0};
    double sampleRate = 0.0;
    bool timeScaleEnabled = false;
    int scrollZoomStepsAccumulated = 0;
    double wheelScrollAccum = 0;   // sub-pixel wheel remainder
    bool zoomFromWheel = false;
    bool annotationCommentsEnabled = false;

    void addPlot(Plot *plot);
    void emitTimeSelection();
    void emitFreqSelection();
    int spectrogramTop();
    double freqAtViewportY(int y);
    int viewportYForFreq(double hz);
    void updateFreqCursorPositions();
    bool cursorPlaceEvent(QEvent::Type type, QMouseEvent *event);
    void applyCursorPlaceDrag(QPoint from, QPoint to);
    void moveNearestCursorTo(int x);
    void updateThresholdPlots();
    void saveViewPosition(double &timeSec, double &freqHz);
    void restoreViewPosition(double timeSec, double freqHz);
    void extractSymbols(std::shared_ptr<AbstractSampleSource> src, bool toClipboard);
    void exportTunerFiltered();
    void exportSpectrogramPng();
    void exportSamples(std::shared_ptr<AbstractSampleSource> src);
    template<typename SOURCETYPE> void exportSamples(std::shared_ptr<AbstractSampleSource> src);
    int plotsHeight();
    size_t samplesPerColumn();
    void updateViewRange(bool reCenter);
    void updateView(bool reCenter = false, bool expanding = false);
    void paintTimeScale(QPainter &painter, QRect &rect, range_t<size_t> sampleRange);
    void updateAnnotationTooltip(QMouseEvent *event);

    int sampleToColumn(size_t sample);
    size_t columnToSample(int col);
};
