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


SyncSink::SyncSink() 
    : GenericProcessor("SyncSink2")
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
}


SyncSink::~SyncSink()
{

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
				std::cout << "stim class specified out of bounds" << std::endl;
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
				std::cout << "empty stim class list" << std::endl;
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
			std::cout << "unable to parse plot param string " << param->getValueAsString() << std::endl;
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
	std::cout << event->getSampleNumber() << " " << event->getChannelInfo()->getSampleRate() << " " << timestamp << " " << std::endl;

	const SpikeChannel* chan = event->getChannelInfo();
	int64 numChannels = chan->getNumChannels();
	int64 numSamples = chan->getTotalSamples();
	int64 spikeChannelIdx = event->getChannelIndex();
	int64 sortedID = event->getSortedId();
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
	if (!nTrialsByStimClass.contains(currentStimClass))
	{
		std::cout << "unregistered stim class " << currentStimClass << std::endl;
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
    std::cout << "received " << message << " " << CoreServices::getSoftwareTimestamp() << std::endl;
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
		for (int stimClass : stimClasses)
		{
			std::cout << stimClass << std::endl;
		}
		std::cout << message << std::endl;
	}
	else if (message.startsWith("TrialStart"))
	{
		
		StringArray tokens;
		tokens.addTokens(message, true);
		/* tokens[0] == TrialStart; tokens[1] == IMGID */
		std::cout << "TrialStart " << tokens[0] << " " << tokens[1] << std::endl;
		//if (conditionMap.contains(tokens[1]))
		//{
		//	currentStimClass = conditionList[conditionMap[tokens[1]]];
		//	nTrials += 1;
		//	nTrialsByStimClass.set(currentStimClass, nTrialsByStimClass[currentStimClass] + 1);
		//}
		//else
		//{
		//	std::cout << "Image ID " << tokens[1] << " not mappable to stimulus class!" << std::endl;
		//}
		//std::cout << "TrialStart for image " << tokens[1] << " at " << timestamp << std::endl;
		//if (thisEditor != nullptr)
		//{
		//	thisEditor->updateLegend();
		//}
	}
	else if (message.startsWith("TrialType")) // Janis Kofiko
	{
		StringArray tokens;
		tokens.addTokens(message, true);
		std::cout << "TrialType " << tokens[0] << " " << tokens[1] << std::endl;
		/* tokens[0] == TrialAlign; tokens[1] == IMGID */
		if (conditionMap.contains(tokens[1]))
		{
			currentStimClass = conditionList[conditionMap[tokens[1]]];
			nTrials += 1;
			nTrialsByStimClass.set(currentStimClass, nTrialsByStimClass[currentStimClass] + 1);
		}
		else
		{
			std::cout << "Image ID " << tokens[1] << " not mappable to stimulus class!" << std::endl;
		}
		std::cout << "TrialType for image " << tokens[1] << " at " << timestamp << std::endl;
		if (canvas != nullptr)
		{
			canvas->updateLegend();
		}
	}
	else if (message.startsWith("TrialAlign"))
	{
		std::cout << "TrialAlign at " << timestamp << std::endl;
		currentTrialStartTime = timestamp;
		inTrial = true;
	}
	else if (message.startsWith("TrialEnd"))
	{
		std::cout << "TrialEnd at " << timestamp << std::endl;
		if (canvas != nullptr) {
			//std::cout << "send update to canvas" << std::endl;
			canvas->updatePlots();
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
				//std::vector<double> conditionTensor = unitTensor[currentStimClass];
				int condition_idx = 0;
				for (int c = 0; c < unitTensor.size(); c++)//std::vector<double> conditionTensor : unitTensor)
				{
					std::vector<double> conditionTensor = unitTensor[c];
					for (double val : conditionTensor)
					{
						val *= double(nTrials) / double(nTrialsByStimClass[c] + 1);
						//							std::cout << val << " ";
					}
					condition_idx += 1;
					//						std::cout << std::endl;
				}
				//					std::cout << std::endl;
			}
			//				std::cout << std::endl;
		}
		//			std::cout << std::endl;
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

bool SyncSink::startAcquisition()
{
	startTimestamp = CoreServices::getSoftwareTimestamp();
	std::cout << startTimestamp << std::endl;
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
		std::cout << "add plot to canvas: " << channel_idx << sorted_id;
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
	std::cout << "SyncSink variables cleared" << std::endl;
}
