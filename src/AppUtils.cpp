#include "cinder/app/App.h"
#if defined( CINDER_MSW )
	#include "cinder/Utilities.h"
	#include "cinder/msw/OutputDebugStringStream.h"
	#include "cinder/app/AppImplMsw.h"
#endif

#include "AppUtils.h"

using namespace ci;

namespace mndl { namespace app {

#ifndef CINDER_MAC
int showMessageBox( const std::string &message, const std::string &title )
{
#if defined( CINDER_MSW )
	// from https://github.com/fieldOfView/Cinder/commit/a5e3ca25032b8318834042a800c295cc050c7480
	return MessageBox( NULL, (LPCWSTR)( toUtf16( message ).c_str() ),
			(LPCWSTR)( toUtf16( title ).c_str() ), MB_OK );
#else
	return 0;
#endif
}
#endif

} } // namespace mndl::app

