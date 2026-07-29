#include "DMCSimulationDataGenerator.h"
#include "DMCAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

namespace {
std::vector<U8> MakePacket(U32 id, U16 type, const std::vector<U8>& data)
{
	std::vector<U8> p;
	p.push_back('D'); p.push_back('F');
	for (unsigned i = 0; i < 4; ++i) p.push_back(static_cast<U8>(id >> (8 * i)));
	p.push_back(static_cast<U8>(type)); p.push_back(static_cast<U8>(type >> 8));
	p.push_back(static_cast<U8>(data.size())); p.push_back(static_cast<U8>(data.size() >> 8));
	p.insert(p.end(), data.begin(), data.end());
	U16 sum1 = 0, sum2 = 0;
	for (size_t i = 0; i < p.size(); ++i) { sum1 = static_cast<U16>((sum1 + p[i]) % 0xff); sum2 = static_cast<U16>((sum2 + sum1) % 0xff); }
	U8 f0 = static_cast<U8>(sum1), f1 = static_cast<U8>(sum2);
	p.push_back(static_cast<U8>(0xff - ((f0 + f1) % 0xff)));
	p.push_back(static_cast<U8>(0xff - ((f0 + p.back()) % 0xff)));
	return p;
}
}

DMCSimulationDataGenerator::DMCSimulationDataGenerator()
:	mSerialText( "My first analyzer, woo hoo!" ),
	mPacket(),
	mStringIndex( 0 )
{
}

DMCSimulationDataGenerator::~DMCSimulationDataGenerator()
{
}

void DMCSimulationDataGenerator::Initialize( U32 simulation_sample_rate, DMCAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mSerialSimulationData.SetChannel( mSettings->mInputChannel );
	mSerialSimulationData.SetSampleRate( simulation_sample_rate );
	mSerialSimulationData.SetInitialBitState( BIT_HIGH );
}

U32 DMCSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mSerialSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		CreateSerialByte();
	}

	*simulation_channel = &mSerialSimulationData;
	return 1;
}

void DMCSimulationDataGenerator::CreateSerialByte()
{
	U32 samples_per_bit = mSimulationSampleRateHz / mSettings->mBitRate;

	if (mPacket.empty() || mStringIndex >= mPacket.size()) {
		std::vector<U8> data;
		data.push_back(0); data.push_back(1); data.push_back(2); data.push_back(3);
		mPacket = MakePacket(1, 0x0020, data);
		mStringIndex = 0;
	}
	U8 byte = mPacket[mStringIndex++];

	//we're currenty high
	//let's move forward a little
	mSerialSimulationData.Advance( samples_per_bit * 10 );

	mSerialSimulationData.Transition();  //low-going edge for start bit
	mSerialSimulationData.Advance( samples_per_bit );  //add start bit time

	// Standard asynchronous UART transmits each byte least-significant bit first.
	U8 mask = 0x1;
	for( U32 i=0; i<8; i++ )
	{
		if( ( byte & mask ) != 0 )
			mSerialSimulationData.TransitionIfNeeded( BIT_HIGH );
		else
			mSerialSimulationData.TransitionIfNeeded( BIT_LOW );

		mSerialSimulationData.Advance( samples_per_bit );
		mask = static_cast<U8>(mask << 1);
	}

	mSerialSimulationData.TransitionIfNeeded( BIT_HIGH ); //we need to end high

	//lets pad the end a bit for the stop bit:
	mSerialSimulationData.Advance( samples_per_bit );
}
