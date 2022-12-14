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
#include "SyncSinkCanvas.h"
#include <zmq.h>

SyncSink::SyncSink() 
    : GenericProcessor("SyncSink2"), Thread("SyncSinkNetworkThread")
{
    addStringParameter(Parameter::GLOBAL_SCOPE,
        "plot",
        "Channel ID, Cluster ID",
        "0");
    //addStringParameter(Parameter::GLOBAL_SCOPE,
    //    "cluster",
    //    "cluster ID",
    //    "0");
    addStringParameter(Parameter::GLOBAL_SCOPE,
        "nbins",
        "Number of Bins",
        "50");
    addStringParameter(Parameter::GLOBAL_SCOPE,
        "binsize",
        "Size of a bin in ms",
        "10");
	context = zmq_ctx_new();
	//socket = zmq_socket(context, ZMQ_SUB);
	socket = zmq_socket(context, ZMQ_REP);
	dataport = 5557;
	//zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);
	const int RECV_TIMEOUT = 10;
	zmq_setsockopt(socket, ZMQ_RCVTIMEO, &RECV_TIMEOUT, sizeof(RECV_TIMEOUT));
	int rc = zmq_bind(socket, "tcp://*:5557");
	std::cout << "SyncSink(): syncsink listening on port 5557 " << rc << " " << zmq_errno() << std::endl;
	startThread();
}


SyncSink::~SyncSink()
{
	if (!stopThread(1000)) {
		std::cerr << "Network thread timeout." << std::endl;
	}
	zmq_close(socket);
	zmq_ctx_destroy(context);
}


AudioProcessorEditor* SyncSink::createEditor()
{
    editor = std::make_unique<SyncSinkEditor>(this);
    return editor.get();
}

void SyncSink::parameterValueChanged(Parameter* param)
{
    if (param->getName().equalsIgnoreCase("plot")) {
		StringArray tokens;
		tokens.addTokens(param->getValueAsString(), ",", "");
		/* tokens[0] == channel_idx; tokens[1] == sorted_id; tokens[2] == stim_class */
		if (tokens.size() == 3)
		{
			if (tokens[2].getIntValue() >= numConditions)
			{
				std::cout << "SyncSink::parameterValueChanged(): stim class specified out of bounds" << std::endl;
				return;
			}
			addPSTHPlot(
				tokens[0].getIntValue(),
				tokens[1].getIntValue(),
				std::vector<int>(1, tokens[2].getIntValue())
			);
		}
		else if (tokens.size() == 2)
		{
			if (numConditions < 0)
			{
				std::cout << "SyncSink::parameterValueChanged(): empty stim class list" << std::endl;
				return;
			}
			addPSTHPlot(
				tokens[0].getIntValue(),
				tokens[1].getIntValue(),
				getStimClasses()
			);
		}
		else
		{
			std::cout << "SyncSink::parameterValueChanged(): unable to parse plot param string " << param->getValueAsString() << std::endl;
		}
    }
    else if (param->getName().equalsIgnoreCase("cluster")) {

    }
    else if (param->getName().equalsIgnoreCase("nbins")) {
		nBins = param->getValueAsString().getIntValue();
		rebin(nBins, binSize);
    }
    else if (param->getName().equalsIgnoreCase("binsize")) {
		binSize = param->getValueAsString().getIntValue();
		rebin(nBins, binSize);
    }
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
	int sampleNum = event->getSampleNumber();
	double sampleRate = event->getChannelInfo()->getSampleRate();
	double sampleTimestamp = (double) sampleNum / (sampleRate / 1000) + startTimestamp;
	int64 timestamp = (int64)sampleTimestamp;
	//std::cout << "SyncSink::handleSpike(): sample num " << event->getSampleNumber() << " timestamp " << timestamp << " " << std::endl;

	const SpikeChannel* chan = event->getChannelInfo();
	int64 numChannels = chan->getNumChannels();
	int64 numSamples = chan->getTotalSamples();
	int64 spikeChannelIdx = event->getChannelIndex();
	int64 sortedID = event->getSortedId();
	//std::cout << "SyncSink::handleSpike(): inTrial" << inTrial << " numConditions " << numConditions << " currentStimClass " << currentStimClass << " currentTrialStartTime " << currentTrialStartTime << std::endl;
	if (!inTrial || numConditions < 0 || currentStimClass < 0 || currentTrialStartTime < 0)
	{
		return; // do not process spike when stimulus is not presented
	}

	/* Store in spikeTensor */
//		std::cout << "spike channel idx " << String(spikeChannelIdx) << std::endl;
	while (spikeTensor.size() < (spikeChannelIdx + 1))
	{
		std::vector<std::vector<std::vector<double>>> thisChannelSpikeTensor;
		//			std::cout << "create channel tensor " << String(spikeTensor.size()) << std::endl;
		spikeTensor.push_back(thisChannelSpikeTensor);
	}

	//		std::cout << "spike sorted id " << String(sortedID) << std::endl;
	while (spikeTensor[spikeChannelIdx].size() < (sortedID + 1))
	{
		std::vector<std::vector<double>> thisUnitSpikeTensor;
		//			std::cout << "create unit tensor" << String(spikeTensor[spikeChannelIdx].size()) << std::endl;
		spikeTensor[spikeChannelIdx].push_back(thisUnitSpikeTensor);
	}

	while (spikeTensor[spikeChannelIdx][sortedID].size() < numConditions)
	{
		std::vector<double> thisConditionSpikeTensor(nBins, (double)0);
		//			std::cout << "create condition tensor" << String(spikeTensor[spikeChannelIdx][sortedID].size()) << std::endl;
		spikeTensor[spikeChannelIdx][sortedID].push_back(thisConditionSpikeTensor);
	}
	//		std::vector<double> thisConditionSpikeTensor = thisUnitSpikeTensor[currentStimClass];

	double offset = double(timestamp - currentTrialStartTime); // milliseconds
	int bin = floor(offset / ((double)binSize));
	//std::cout << "SyncSink::handleSpike(): sample num " << event->getSampleNumber() << " timestamp " << timestamp << " offset " << offset << " bin " << bin << std::endl;
	if (!nTrialsByStimClass.contains(currentStimClass))
	{
		std::cout << "SyncSink::handleSpike(): unregistered stim class " << currentStimClass << std::endl;
		return;
	}
	if (bin < nBins)
	{
		spikeTensor[spikeChannelIdx][sortedID][currentStimClass][bin] += double(1) / double(nTrialsByStimClass[currentStimClass]); // assignment not working
//			std::cout << spikeTensor[spikeChannelIdx][sortedID][currentStimClass][bin] << " ";
	}
}


void SyncSink::handleBroadcastMessage(String message)
{
    //std::cout << "SyncSink::handleBroadcastMessage(): received " << message << " " << CoreServices::getSoftwareTimestamp() << std::endl;
	int64 timestamp = CoreServices::getSoftwareTimestamp();
	/* Parse Kofiko */
	// Beginning of trial: add conditions
	//std::cout << text << std::endl;
	if (message.startsWith("ClearDesign"))
	{
		clearVars();
		if (canvas != nullptr)
		{
			canvas->update();
		}
	}
	else if (message.startsWith("AddCondition"))
	{
		StringArray tokens;
		tokens.addTokens(message, true);
		/* tokens[0] == AddCondition; tokens[1] == Name; tokens[2] == STIMCLASS;
		   tokens[3] == Visible; tokens[4] == 1; tokens[5] == TrialTypes; */
		for (int i = 6; i < tokens.size(); i++)
		{
			//std::cout << tokens[i] << std::endl;
			conditionMap.set(tokens[i], tokens[2]);

		}
		conditionList.set(tokens[2], numConditions);
		conditionListInverse.set(numConditions, tokens[2]);
		nTrialsByStimClass.set(numConditions, 0);
		stimClasses.push_back(numConditions);
		numConditions += 1;
		if (canvas != nullptr)
		{
			canvas->update();
		}
		//for (int stimClass : stimClasses)
		//{
		//	std::cout << "SyncSink::handleBroadcastMessage(): stimClass = " << stimClass << std::endl;
		//}
		std::cout << "SyncSink::handleBroadcastMessage(): add stimClass " << numConditions << std::endl;
	}
	else if (message.startsWith("TrialStart")) // Jialiang / Berkeley Kofiko -- Sept. 2022
	{
		
		StringArray tokens;
		tokens.addTokens(message, true);
		/* tokens[0] == TrialStart; tokens[1] == IMGID */
		//std::cout << "SyncSink::handleBroadcastMessage(): TrialStart " << tokens[0] << " " << tokens[1] << std::endl;
		if (conditionMap.contains(tokens[1]))
		{
			currentStimClass = conditionList[conditionMap[tokens[1]]];
			nTrials += 1;
			nTrialsByStimClass.set(currentStimClass, nTrialsByStimClass[currentStimClass] + 1);
		}
		else
		{
			std::cout << "SyncSink::handleBroadcastMessage(): Image ID " << tokens[1] << " not mappable to stimulus class!" << std::endl;
		}
		//std::cout << "SyncSink::handleBroadcastMessage(): TrialStart for image " << tokens[1] << " at " << timestamp << std::endl;
		if (canvas != nullptr)
		{
			canvas->updateLegend();
		}
	}
	else if (message.startsWith("TrialType")) // Janis Kofiko -- deprecated
	{
		StringArray tokens;
		tokens.addTokens(message, true);
		//std::cout << "SyncSink::handleBroadcastMessage(): TrialType " << tokens[0] << " " << tokens[1] << std::endl;
		/* tokens[0] == TrialAlign; tokens[1] == IMGID */
		if (conditionMap.contains(tokens[1]))
		{
			currentStimClass = conditionList[conditionMap[tokens[1]]];
			nTrials += 1;
			nTrialsByStimClass.set(currentStimClass, nTrialsByStimClass[currentStimClass] + 1);
		}
		else
		{
			std::cout << "SyncSink::handleBroadcastMessage(): Image ID " << tokens[1] << " not mappable to stimulus class!" << std::endl;
		}
		//std::cout << "SyncSink::handleBroadcastMessage(): TrialType for image " << tokens[1] << " at " << timestamp << std::endl;
		if (canvas != nullptr)
		{
			canvas->updateLegend();
		}
	}
	else if (message.startsWith("TrialAlign"))
	{
		//std::cout << "SyncSink::handleBroadcastMessage(): TrialAlign at " << timestamp << std::endl;
		currentTrialStartTime = timestamp;
		inTrial = true;
	}
	else if (message.startsWith("TrialEnd"))
	{
		//std::cout << "SyncSink::handleBroadcastMessage(): TrialEnd at " << timestamp << std::endl;
		if (canvas != nullptr) {
			//std::cout << "send update to canvas" << std::endl;
			canvas->updatePlots();
			canvas->repaint();
		}
		//if (!nTrialsByStimClass.contains(currentStimClass))
		//{
		//	std::cout << "unregistered stim class " << currentStimClass << std::endl;
		//	return;
		//}
		for (std::vector<std::vector<std::vector<double>>> channelTensor : spikeTensor)
		{
			for (std::vector<std::vector<double>> unitTensor : channelTensor)
			{

				std::vector<double> conditionTensor = unitTensor[currentStimClass];
				for (double val : conditionTensor)
				{
					val *= double(nTrialsByStimClass[currentStimClass]) / double(nTrialsByStimClass[currentStimClass] + 1);
					//std::cout << val << " ";
				}
				//std::cout << std::endl;
			}
			//std::cout << std::endl;
		}
		//std::cout << std::endl;
		currentTrialStartTime = -1;
		currentStimClass = -1;
		inTrial = false;
	}
}


void SyncSink::saveCustomParametersToXml(XmlElement* parentElement)
{

}


void SyncSink::loadCustomParametersFromXml(XmlElement* parentElement)
{

}

void SyncSink::run()
{
	HeapBlock<char> buf(2048);
	while (!threadShouldExit()) {
		int res = zmq_recv(socket, buf, 2048, 0);
		if (res == -1) {
			//std::cout << "SyncSink::run(): failed to receive message" << std::endl;
			continue;
		}
		String msg = String::fromUTF8(buf, res);
		handleBroadcastMessage(msg);
		zmq_send(socket, "", 0, 0);
	}
}

bool SyncSink::startAcquisition()
{
	startTimestamp = CoreServices::getSoftwareTimestamp();
	std::cout << "SyncSink::startAcquisition():" << startTimestamp << std::endl;
	return true;
}

std::vector<double> SyncSink::getHistogram(int channel_idx, int sorted_id, int stim_class)
{
	if (channel_idx >= spikeTensor.size()) {
		return std::vector<double>(nBins, 0);
	}
	if (sorted_id >= spikeTensor[channel_idx].size()) {
		return std::vector<double>(nBins, 0);
	}
	if (stim_class >= spikeTensor[channel_idx][sorted_id].size())
	{
		return std::vector<double>(nBins, 0);
	}
	return spikeTensor[channel_idx][sorted_id][stim_class];
}

int SyncSink::getNTrial()
{
	return nTrials;
}

void SyncSink::setCanvas(SyncSinkCanvas* c)
{
	canvas = c;
}

void SyncSink::setEditor(SyncSinkEditor* e)
{
	thisEditor = e;
}

void SyncSink::addPSTHPlot(int channel_idx, int sorted_id, std::vector<int> stimClasses)
{
	if (canvas != nullptr) {
		std::cout << "SyncSink::addPSTHPlot(): add plot to canvas: " << channel_idx << sorted_id;
		for (int stim_class : stimClasses)
		{
			std::cout << stim_class << "(" << conditionListInverse[stim_class] << ") ";

		}
		std::cout << std::endl;
		canvas->addPlot(channel_idx, sorted_id, stimClasses);
	}
}

void SyncSink::resetTensor()
{
	int ch_idx = 0;
	for (std::vector<std::vector<std::vector<double>>> channelTensor : spikeTensor)
	{
		int un_idx = 0;
		for (std::vector<std::vector<double>> unitTensor : spikeTensor[ch_idx])
		{
			int cond_idx = 0;
			for (std::vector<double> conditionTensor : spikeTensor[ch_idx][un_idx])
			{
				for (int i = 0; i < spikeTensor[ch_idx][un_idx][cond_idx].size(); i++)
				{
					spikeTensor[ch_idx][un_idx][cond_idx][i] = 0;
				}
				while (spikeTensor[ch_idx][un_idx][cond_idx].size() < nBins)
				{
					spikeTensor[ch_idx][un_idx][cond_idx].push_back(0);
				}
				cond_idx++;
			}
			un_idx++;
		}
		ch_idx++;
	}
	nTrials = 0;
	if (canvas != nullptr)
	{
		canvas->updatePlots();
		canvas->update();
		canvas->updateLegend();
	}
}

void SyncSink::rebin(int n_bins, int bin_size)
{
	nBins = n_bins;
	binSize = bin_size;
	if (canvas != nullptr)
	{
		canvas->updatePlots();
		canvas->update();
		canvas->updateLegend();
	}
}

String SyncSink::getStimClassLabel(int stim_class)
{
	if (conditionListInverse.contains(stim_class))
	{
		return conditionListInverse[stim_class];
	}
	return "";
}

int SyncSink::getNBins()
{
	return nBins;
}

int SyncSink::getBinSize()
{
	return binSize;
}

std::vector<int> SyncSink::getStimClasses()
{
	return stimClasses;
}

void SyncSink::clearVars()
{
	conditionMap.clear();
	conditionList.clear();
	conditionListInverse.clear();
	nTrialsByStimClass.clear();
	spikeTensor.clear();
	stimClasses.clear();
	numConditions = 0;
	nTrials = 0;
	currentStimClass = -1;
	currentTrialStartTime = -1;
	inTrial = false;
	std::cout << "SyncSink::clearVars(): SyncSink variables cleared" << std::endl;
}
