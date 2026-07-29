#ifndef DMC_SIMULATION_DATA_GENERATOR
#define DMC_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
#include <vector>
class DMCAnalyzerSettings;

class DMCSimulationDataGenerator
{
public:
	DMCSimulationDataGenerator();
	~DMCSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, DMCAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	DMCAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	std::vector<U8> mPacket;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //DMC_SIMULATION_DATA_GENERATOR
