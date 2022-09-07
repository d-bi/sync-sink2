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

#include "SyncSink.h"

#include "SyncSinkEditor.h"


SyncSink::SyncSink() 
    : GenericProcessor("SyncSink2")
{

}


SyncSink::~SyncSink()
{

}


AudioProcessorEditor* SyncSink::createEditor()
{
    editor = std::make_unique<SyncSinkEditor>(this);
    return editor.get();
}


void SyncSink::updateSettings()
{


}


void SyncSink::process(AudioBuffer<float>& buffer)
{

    checkForEvents(true);
	 
}


void SyncSink::handleTTLEvent(TTLEventPtr event)
{

}


void SyncSink::handleSpike(SpikePtr event)
{

}


void SyncSink::handleBroadcastMessage(String message)
{

}


void SyncSink::saveCustomParametersToXml(XmlElement* parentElement)
{

}


void SyncSink::loadCustomParametersFromXml(XmlElement* parentElement)
{

}