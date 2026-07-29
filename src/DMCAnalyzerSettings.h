#ifndef DMC_ANALYZER_SETTINGS
#define DMC_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class DMCAnalyzerSettings : public AnalyzerSettings
{
public:
	DMCAnalyzerSettings();
	virtual ~DMCAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;
	U32 mBitRate;
	bool mShowSerialBytes;

protected:
	AnalyzerSettingInterfaceChannel	mInputChannelInterface;
	AnalyzerSettingInterfaceInteger	mBitRateInterface;
	AnalyzerSettingInterfaceBool mShowSerialBytesInterface;
};

#endif //DMC_ANALYZER_SETTINGS
