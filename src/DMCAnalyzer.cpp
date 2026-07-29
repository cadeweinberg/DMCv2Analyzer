#include "DMCAnalyzer.h"
#include "DMCAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

DMCAnalyzer::DMCAnalyzer()
:	Analyzer2(),  
	mSettings(),
	mSerial(nullptr),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( &mSettings );
	UseFrameV2();
}

DMCAnalyzer::~DMCAnalyzer()
{
	KillThread();
}

void DMCAnalyzer::SetupResults()
{
	// SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the results each time.
	mResults.reset(new DMCAnalyzerResults( this, &mSettings ));
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings.mInputChannel );
}

void DMCAnalyzer::WorkerThread()
{
	U32 sample_rate_hz = GetSampleRate();

	mSerial = GetAnalyzerChannelData( mSettings.mInputChannel );

	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();

	const double samples_per_bit = double( sample_rate_hz ) / double( mSettings.mBitRate );
	DMCProtocol::StreamParser parser;

	for( ; ; )
	{
		if( mSerial->GetBitState() == BIT_LOW ) mSerial->AdvanceToNextEdge();
		mSerial->AdvanceToNextEdge(); //falling edge -- beginning of the start bit

		U64 starting_sample = mSerial->GetSampleNumber();

		U8 data = 0;
		for( U32 i=0; i<8; i++ )
		{
			// Use absolute, rounded positions instead of repeatedly advancing by
			// truncated samples_per_bit. This prevents baud/sample-rate drift.
			const U64 bit_center = starting_sample + static_cast<U64>((i + 1.5) * samples_per_bit + 0.5);
			mSerial->AdvanceToAbsPosition( bit_center );
			//let's put a dot exactly where we sample this bit:
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings.mInputChannel );

			// Standard asynchronous UART transmits each byte least-significant bit first.
			if( mSerial->GetBitState() == BIT_HIGH ) data |= static_cast<U8>(1 << i);
		}
		// Stop bit is sampled one bit after the final data bit.
		const U64 stop_center = starting_sample + static_cast<U64>(9.5 * samples_per_bit + 0.5);
		mSerial->AdvanceToAbsPosition( stop_center );
		U64 ending_sample = mSerial->GetSampleNumber();
		DMCProtocol::ByteSample byte{ data, starting_sample, ending_sample, mSerial->GetBitState() != BIT_HIGH };
		std::vector<DMCProtocol::Packet> packets;
		mResults->AddByteResult( byte );
		parser.Push( byte, packets );
		for( std::vector<DMCProtocol::Packet>::const_iterator it = packets.begin(); it != packets.end(); ++it )
			mResults->AddPacket( *it );
		ReportProgress( ending_sample );
	}
}

bool DMCAnalyzer::NeedsRerun()
{
	return false;
}

U32 DMCAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 DMCAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings.mBitRate * 4;
}

const char* DMCAnalyzer::GetAnalyzerName() const
{
	return "DMC";
}

const char* GetAnalyzerName()
{
	return "DMC";
}

Analyzer* CreateAnalyzer()
{
	return new DMCAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
