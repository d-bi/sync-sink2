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
#ifndef SYNCSINK_H_DEFINED
#define SYNCSINK_H_DEFINED

#include <ProcessorHeaders.h>


/** 
	A plugin that includes a canvas for displaying incoming data
	or an extended settings interface.
*/

class SyncSinkCanvas;
class SyncSinkEditor;

class SyncSink : public GenericProcessor
{
public:
	/** The class constructor, used to initialize any members.*/
	SyncSink();

	/** The class destructor, used to deallocate memory*/
	~SyncSink();

	/** If the processor has a custom editor, this method must be defined to instantiate it. */
	AudioProcessorEditor* createEditor() override;

	void parameterValueChanged(Parameter* param) override;

	/** Called every time the settings of an upstream plugin are changed.
		Allows the processor to handle variations in the channel configuration or any other parameter
		passed through signal chain. The processor can use this function to modify channel objects that
		will be passed to downstream plugins. */
	void updateSettings() override;

	/** Defines the functionality of the processor.
		The process method is called every time a new data buffer is available.
		Visualizer plugins typically use this method to send data to the canvas for display purposes */
	void process(AudioBuffer<float>& buffer) override;

	/** Handles events received by the processor
		Called automatically for each received event whenever checkForEvents() is called from
		the plugin's process() method */
	void handleTTLEvent(TTLEventPtr event) override;

	/** Handles spikes received by the processor
		Called automatically for each received spike whenever checkForEvents(true) is called from 
		the plugin's process() method */
	void handleSpike(SpikePtr spike) override;

	/** Handles broadcast messages sent during acquisition
		Called automatically whenever a broadcast message is sent through the signal chain */
	void handleBroadcastMessage(String message) override;

	/** Saving custom settings to XML. This method is not needed to save the state of
		Parameter objects */
	void saveCustomParametersToXml(XmlElement* parentElement) override;

	/** Load custom settings from XML. This method is not needed to load the state of
		Parameter objects*/
	void loadCustomParametersFromXml(XmlElement* parentElement) override;

	bool startAcquisition() override;

	//SyncSinkCanvas* syncSinkCanvas = nullptr;
	std::vector<double> getHistogram(int channel_idx, int sorted_id, int stim_class);
	int getNTrial();
	void setCanvas(SyncSinkCanvas* c);
	void setEditor(SyncSinkEditor* e);
	void addPSTHPlot(int channel_idx, int sorted_id, std::vector<int> stimClasses);
	void resetTensor();
	void rebin(int n_bins, int bin_size);
	String getStimClassLabel(int stim_class);
	int getNBins();
	int getBinSize();
	std::vector<int> getStimClasses();
	void clearVars();
	int numConditions = -1;
	SyncSinkCanvas* canvas = nullptr;
	SyncSinkEditor* thisEditor = nullptr;


private:

	/** Generates an assertion if this class leaks */
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyncSink);

	HashMap<String, String> conditionMap; // hashmap for image ids
	HashMap<String, int> conditionList; // hashmap for condition indexing
	HashMap<int, String> conditionListInverse; // hashmap for index to condition string
	HashMap<int, int> nTrialsByStimClass; // hashmap storing num trials for each condition
	std::vector<int> stimClasses;
	int currentStimClass = -1;
	int64 currentTrialStartTime = -1;
	bool inTrial = false;
	std::vector<std::vector<std::vector<std::vector<double>>>> spikeTensor; // n_channels * n_units * n_stim_classes * n_bins tensor for spike counts

	int nBins = 50; // default num bins
	int binSize = 10; // default bin sizes
	int nTrials = 0;
	//void* context;
	//void* socket;
	//int dataport;

	int64 startTimestamp = 0; // software timestamp at start of acquisition
	int sampleRate = 0; // sample rate

};

#endif // SyncSink_H_DEFINED