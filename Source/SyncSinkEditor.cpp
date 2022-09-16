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


#include "SyncSinkEditor.h"

#include "SyncSinkCanvas.h"
#include "SyncSink.h"

#include <stdio.h>


SyncSinkEditor::SyncSinkEditor(GenericProcessor* p)
    : VisualizerEditor(p, "Visualizer", 200), syncSinkCanvas(nullptr)
{
    desiredWidth = 250;
    addTextBoxParameterEditor("plot", 20, 20);
    //addTextBoxParameterEditor("cluster", 120, 20);
    addTextBoxParameterEditor("nbins", 20, 60);
    addTextBoxParameterEditor("binsize", 120, 60);
    //addSelectedChannelsParameterEditor("Channels", 20, 105);

}

Visualizer* SyncSinkEditor::createNewCanvas()
{
    return new SyncSinkCanvas((SyncSink*) getProcessor());;
}

void SyncSinkEditor::updateSettings()
{
}
