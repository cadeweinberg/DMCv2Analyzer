#include "DMCAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "DMCAnalyzer.h"
#include "DMCAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <sstream>

DMCAnalyzerResults::DMCAnalyzerResults( DMCAnalyzer* analyzer, DMCAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer ),
	mPackets()
{
}

DMCAnalyzerResults::~DMCAnalyzerResults()
{
}

void DMCAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	if (frame_index < mPackets.size()) {
		const DMCProtocol::Packet& p = mPackets[frame_index];
		std::ostringstream s;
		s << p.type_name << " 0x" << std::hex << p.id << " " << p.status;
		AddResultString(s.str().c_str());
	} else AddResultString( "DMC packet" );
}

void DMCAnalyzerResults::AddPacket( const DMCProtocol::Packet& packet )
{
	mPackets.push_back(packet);
#ifdef LOGIC2
	FrameV2 frame;
	frame.AddInteger( "ID", packet.id );
	frame.AddInteger( "Type", packet.type );
	frame.AddString( "Message", packet.type_name.c_str() );
	frame.AddInteger( "Length", packet.length );
	frame.AddString( "Status", packet.status.c_str() );
	frame.AddString( "Direction", packet.direction.c_str() );
	frame.AddBoolean( "ChecksumValid", packet.checksum_valid );
	frame.AddBoolean( "FramingError", packet.framing_error );
	frame.AddByteArray( "Raw", packet.raw.empty() ? 0 : &packet.raw[0], packet.raw.size() );
	for( std::vector<DMCProtocol::Field>::const_iterator it = packet.fields.begin(); it != packet.fields.end(); ++it )
		frame.AddString( it->key.c_str(), it->value.c_str() );
	AddFrameV2( frame, "DMC", packet.start_sample, packet.end_sample );
	CommitResults();
#else
	Frame frame;
	frame.mStartingSampleInclusive = packet.start_sample;
	frame.mEndingSampleInclusive = packet.end_sample;
	frame.mData1 = packet.type;
	frame.mData2 = packet.id;
	frame.mFlags = packet.checksum_valid ? 0 : DISPLAY_AS_ERROR_FLAG;
	AddFrame( frame );
	CommitResults();
#endif
}

void DMCAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Message,ID,Type,Length,Status,ChecksumValid,Fields,Raw" << std::endl;

	U64 num_frames = mPackets.size();
	for( U32 i=0; i < num_frames; i++ )
	{
		char time_str[128];
		AnalyzerHelpers::GetTimeString( mPackets[i].start_sample, trigger_sample, sample_rate, time_str, 128 );
		const DMCProtocol::Packet& p = mPackets[i];
		file_stream << time_str << "," << p.type_name << "," << p.id << "," << p.type << "," << p.length << "," << p.status << "," << (p.checksum_valid ? "true" : "false") << ",\"";
		for (size_t f = 0; f < p.fields.size(); ++f) {
			if (f) file_stream << ";";
			file_stream << p.fields[f].key << "=" << p.fields[f].value;
		}
		file_stream << "\",\"";
		for (size_t b = 0; b < p.raw.size(); ++b) file_stream << (b ? " " : "") << std::hex << std::uppercase << static_cast<unsigned>(p.raw[b]);
		file_stream << std::dec << std::nouppercase << "\"" << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void DMCAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	ClearTabularText();
	if (frame_index < mPackets.size()) AddTabularText(mPackets[frame_index].type_name.c_str(), " ", mPackets[frame_index].status.c_str());
#endif
}

void DMCAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void DMCAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}
