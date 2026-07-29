#ifndef DMC_ANALYZER_RESULTS
#define DMC_ANALYZER_RESULTS

#include <AnalyzerResults.h>
#include "DMCProtocol.h"

class DMCAnalyzer;
class DMCAnalyzerSettings;

class DMCAnalyzerResults : public AnalyzerResults
{
public:
	DMCAnalyzerResults( DMCAnalyzer* analyzer, DMCAnalyzerSettings* settings );
	virtual ~DMCAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );
	void AddPacket( const DMCProtocol::Packet& packet );

protected: //functions

protected:  //vars
	DMCAnalyzerSettings* mSettings;
	DMCAnalyzer* mAnalyzer;
	std::vector<DMCProtocol::Packet> mPackets;
};

#endif //DMC_ANALYZER_RESULTS
