#include "CaptureParams.h"

using namespace ci;
using namespace std;

namespace mndl {

params::PInterfaceGl CaptureParams::mParams   = params::PInterfaceGl();

int CaptureParams::mBrightness                = -1;
int CaptureParams::mContrast                  = -1;
int CaptureParams::mSharpness                 = -1;
int CaptureParams::mGamma                     = -1;
int CaptureParams::mBacklightCompensation     = -1;
int CaptureParams::mGain                      = -1;

CaptureParams::CaptureParams()
: Capture()
{
}

CaptureParams::CaptureParams( int32_t width, int32_t height, const ci::Capture::DeviceRef device )
: Capture( width, height, device )
{
}

void CaptureParams::setup()
{
#ifdef CINDER_MSW
	mParams = params::PInterfaceGl( "Capture settings", Vec2i( 250, 200 ) );
	mParams.addPersistentSizeAndPosition();
#endif
}

void CaptureParams::buildParams()
{
#ifdef CINDER_MSW
	params::PInterfaceGl::save();

	removeParams();

	buildParam( Capture::Device::SFT_Brightness           , &mBrightness           , "Brightness"            );
	buildParam( Capture::Device::SFT_Contrast             , &mContrast             , "Contrast"              );
	buildParam( Capture::Device::SFT_Sharpness            , &mSharpness            , "Sharpness"             );
	buildParam( Capture::Device::SFT_Gamma                , &mGamma                , "Gamma"                 );
	buildParam( Capture::Device::SFT_BacklightCompensation, &mBacklightCompensation, "BacklightCompensation" );
	buildParam( Capture::Device::SFT_Gain                 , &mGain                 , "Gain"                  );
#endif
}

void CaptureParams::updateParams()
{
#ifdef CINDER_MSW
	static int brightnessLast            = -1;
	static int contrastLast              = -1;
	static int sharpnessLast             = -1;
	static int gammaLast                 = -1;
	static int backlightCompensationLast = -1;
	static int gainLast                  = -1;

	updateParam( Capture::Device::SFT_Brightness           , &mBrightness           , &brightnessLast            );
	updateParam( Capture::Device::SFT_Contrast             , &mContrast             , &contrastLast              );
	updateParam( Capture::Device::SFT_Sharpness            , &mSharpness            , &sharpnessLast             );
	updateParam( Capture::Device::SFT_Gamma                , &mGamma                , &gammaLast                 );
	updateParam( Capture::Device::SFT_BacklightCompensation, &mBacklightCompensation, &backlightCompensationLast );
	updateParam( Capture::Device::SFT_Gain                 , &mGain                 , &gainLast                  );
#endif
}


#ifdef CINDER_MSW
void CaptureParams::buildParam( Capture::Device::SettingsFilterType settingsType, int *settingsValue, const string &name )
{
	long value = 0, min = 0, max = 0, step = 0, def = 0;

	if( getDevice()->getSettingsFilter( settingsType, min, max, step, value, def ))
	{
		mParams.addPersistentParam( name, settingsValue, (int)def, getMinMaxStepString( min, max, step ));
		*settingsValue = math<int>::clamp( *settingsValue , min, max );
	}
}

void CaptureParams::updateParam( Capture::Device::SettingsFilterType settingsType, int *settingsValue, int *settingsValueLast )
{
	if( *settingsValueLast != *settingsValue )
	{
		getDevice()->setSettingsFilter( settingsType, *settingsValue );
		*settingsValueLast = *settingsValue;
	}
}
#endif

void CaptureParams::removeParams()
{
#ifdef CINDER_MSW
	mParams.removeParam( "Brightness"            );
	mParams.removeParam( "Contrast"              );
	mParams.removeParam( "Sharpness"             );
	mParams.removeParam( "Gamma"                 );
	mParams.removeParam( "BacklightCompensation" );
	mParams.removeParam( "Gain"                  );
#endif
}

string CaptureParams::getMinMaxStepString( int min, int max, int step )
{
	stringstream st;
	st << "min=" << min << " max=" << max << " step=" << step;
	return st.str();
}

} // namspace mndl

