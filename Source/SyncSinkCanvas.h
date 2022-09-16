/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2022 Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef VISUALIZERCANVAS_H_INCLUDED
#define VISUALIZERCANVAS_H_INCLUDED

#include <VisualizerWindowHeaders.h>

class SyncSink;
class SyncSinkDisplay;
class PSTHPlot;
/**
* 
	Draws data in real time

*/
class SyncSinkCanvas : public Visualizer
{
public:

	/** Constructor */
	SyncSinkCanvas(SyncSink* s);

	/** Destructor */
	~SyncSinkCanvas();

	/** Updates boundaries of sub-components whenever the canvas size changes */
	void resized() override;

	/** Called when the visualizer's tab becomes visible again */
	void refreshState() override;

	/** Updates settings */
	void update() override;

	/** Called instead of "repaint()" to avoid re-painting sub-components*/
	void refresh() override;

	/** Draws the canvas background */
	void paint(Graphics& g) override;

	void updatePlots();
	void addPlot(int channel_idx, int sorted_id, std::vector<int> stimClasses);

	std::vector<Colour> colorList = {
		Colour(30,118,179), Colour(255,126,13), Colour(43,159,43),
		Colour(213,38,39), Colour(147,102,188), Colour(139,85,74),
		Colour(226,118,193), Colour(126,126,126), Colour(187,188,33),
		Colour(22,189,206) };

	void updateLegend();

private:

	/** Pointer to the processor class */
	SyncSink* processor;

	/** Class for plotting data */
	InteractivePlot plt;

	ScopedPointer<Viewport> viewport;
	ScopedPointer<SyncSinkDisplay> display;

	/** Generates an assertion if this class leaks */
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyncSinkCanvas);
};

class SyncSinkDisplay : public Component
{
public:
    SyncSinkDisplay(SyncSink* s, SyncSinkCanvas* c, Viewport* v);
    ~SyncSinkDisplay();
    void paint(Graphics& g);
    void resized();

    void removePlots();
    void clear();

    void updatePlots();
    void addPSTHPlot(int channel_idx, int sorted_id, std::vector<int> stimClasses);

private:
    SyncSink* processor;
    int plotCounter = 0;
    SyncSinkCanvas* canvas;
    Viewport* viewport;
    OwnedArray<PSTHPlot> plots;

};

class PSTHPlot : public Component
{
public:
    PSTHPlot(SyncSink* s, SyncSinkCanvas* c, SyncSinkDisplay* d,
        int channel_idx, int sorted_id, int stim_class, int identifier);
    PSTHPlot(SyncSink* s, SyncSinkCanvas* c, SyncSinkDisplay* d,
        int channel_idx, int sorted_id, std::vector<int> stimClasses, int identifier);
    ~PSTHPlot();

    void paint(Graphics& g);
    void resized();
    void clearPlot();

    //void updatePlot();

    int channel_idx;
    int sorted_id;
    //int stim_class;
    std::vector<int> stimClasses;
    int identifier;
private:
    SyncSink* processor;
    SyncSinkDisplay* display;
    SyncSinkCanvas* canvas;
    Font font;
    String name;
    std::vector<double> histogram;
    bool alive;
};

#endif // SPECTRUMCANVAS_H_INCLUDED