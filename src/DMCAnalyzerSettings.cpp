#include "DMCAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


DMCAnalyzerSettings::DMCAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 115200 ),
	mShowSerialBytes( false ),
	mInputChannelInterface(),
	mBitRateInterface(),
	mShowSerialBytesInterface()
{
	mInputChannelInterface.SetTitleAndTooltip( "Serial", "Standard DMC" );
	mInputChannelInterface.SetChannel( mInputChannel );

	mBitRateInterface.SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface.SetMax( 6000000 );
	mBitRateInterface.SetMin( 1 );
	mBitRateInterface.SetInteger( mBitRate );
	mShowSerialBytesInterface.SetTitleAndTooltip( "Show Serial Bytes", "Emit diagnostic serial_byte results in the data table." );
	mShowSerialBytesInterface.SetValue( mShowSerialBytes );

	AddInterface( &mInputChannelInterface );
	AddInterface( &mBitRateInterface );
	AddInterface( &mShowSerialBytesInterface );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

DMCAnalyzerSettings::~DMCAnalyzerSettings()
{
}

bool DMCAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface.GetChannel();
	mBitRate = mBitRateInterface.GetInteger();
	mShowSerialBytes = mShowSerialBytesInterface.GetValue();

	ClearChannels();
	AddChannel( mInputChannel, "DMC", true );

	return true;
}

void DMCAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface.SetChannel( mInputChannel );
	mBitRateInterface.SetInteger( mBitRate );
	mShowSerialBytesInterface.SetValue( mShowSerialBytes );
}

void DMCAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;
	text_archive >> mShowSerialBytes;

	ClearChannels();
	AddChannel( mInputChannel, "DMC", true );

	UpdateInterfacesFromSettings();
}

const char* DMCAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;
	text_archive << mShowSerialBytes;

	return SetReturnString( text_archive.GetString() );
}
