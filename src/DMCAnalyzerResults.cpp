#include "DMCAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "DMCAnalyzer.h"
#include "DMCAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

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
		std::ostringstream header;
		header << p.type_name << "  ID=0x" << std::hex << p.id << "  Length=" << std::dec << p.length << "  " << p.status;
		AddResultString(header.str().c_str());

		for (std::vector<DMCProtocol::Field>::const_iterator it = p.fields.begin(); it != p.fields.end(); ++it) {
			std::string field = it->key + "=" + it->value;
			AddResultString(field.c_str());
		}

	} else AddResultString( "DMC packet" );
}

void DMCAnalyzerResults::AddByteResult( const DMCProtocol::ByteSample& byte )
{
#ifdef LOGIC2
	FrameV2 frame;
	frame.AddByte( "Data", byte.value );
	frame.AddBoolean( "FramingError", byte.framing_error );
	AddFrameV2( frame, "serial_byte", byte.start, byte.end );
	CommitResults();
#else
	Frame frame;
	frame.mStartingSampleInclusive = byte.start;
	frame.mEndingSampleInclusive = byte.end;
	frame.mData1 = byte.value;
	frame.mFlags = byte.framing_error ? DISPLAY_AS_ERROR_FLAG : 0;
	AddFrame( frame );
	CommitResults();
#endif
}

void DMCAnalyzerResults::AddPacket( const DMCProtocol::Packet& packet )
{
	mPackets.push_back(packet);
	// FrameV1 is still required for waveform bubbles and the legacy export path.
	Frame legacy_frame;
	legacy_frame.mStartingSampleInclusive = packet.start_sample;
	legacy_frame.mEndingSampleInclusive = packet.end_sample;
	legacy_frame.mData1 = packet.type;
	legacy_frame.mData2 = packet.id;
	legacy_frame.mFlags = (packet.checksum_valid && !packet.framing_error && !packet.truncated) ? 0 : DISPLAY_AS_ERROR_FLAG;
	AddFrame( legacy_frame );
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
	for( std::vector<DMCProtocol::Field>::const_iterator it = packet.fields.begin(); it != packet.fields.end(); ++it )
		frame.AddString( it->key.c_str(), it->value.c_str() );
	AddFrameV2( frame, "DMC", packet.start_sample, packet.end_sample );
	CommitResults();
#else
	CommitResults();
#endif
}

void DMCAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Message,ID,Type,Length,Status,ChecksumValid,Fields" << std::endl;

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
		file_stream << "\"" << std::endl;

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
	if (frame_index < mPackets.size()) {
		const DMCProtocol::Packet& p = mPackets[frame_index];
		std::ostringstream text;
		text << p.type_name << " ID=0x" << std::hex << p.id << " Length=" << std::dec << p.length << " " << p.status;
		for (std::vector<DMCProtocol::Field>::const_iterator it = p.fields.begin(); it != p.fields.end(); ++it)
			text << " " << it->key << "=" << it->value;
		AddTabularText(text.str().c_str());
	}
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
