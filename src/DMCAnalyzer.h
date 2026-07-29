#ifndef DMC_ANALYZER_H
#define DMC_ANALYZER_H

#include <Analyzer.h>
#include "DMCAnalyzerSettings.h"
#include "DMCAnalyzerResults.h"
#include "DMCSimulationDataGenerator.h"
#include "DMCProtocol.h"
#include <memory>

class DMCAnalyzer : public Analyzer2
{
public:
	DMCAnalyzer();
	virtual ~DMCAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	DMCAnalyzerSettings mSettings;
	std::unique_ptr<DMCAnalyzerResults> mResults;
	AnalyzerChannelData* mSerial;
	U32 mSamplesPerBit;

	DMCSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //DMC_ANALYZER_H
