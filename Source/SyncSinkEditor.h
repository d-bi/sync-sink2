/*
	------------------------------------------------------------------

	This file is part of the Open Ephys GUI
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

//This prevents include loops. We recommend changing the macro to a name suitable for your plugin
#ifndef SYNCSINKEDITOR_H_DEFINED
#define SYNCSINKEDITOR_H_DEFINED

#include <VisualizerEditorHeaders.h>

/** 
	The editor for the SyncSink

	Includes buttons for opening the canvas in a tab or window
*/

class SyncSinkEditor : public VisualizerEditor
{
public:

	/** Constructor */
	SyncSinkEditor(GenericProcessor* parentNode);

	/** Destructor */
	~SyncSinkEditor() { }

	/** Creates the canvas */
	Visualizer* createNewCanvas();

private:

	/** Generates an assertion if this class leaks */
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyncSinkEditor);
};


#endif // SyncSinkEDITOR_H_DEFINED