#include "cinder/app/App.h"
#if defined( CINDER_MSW )
	#include "cinder/msw/OutputDebugStringStream.h"
	#include "cinder/app/AppImplMsw.h"
#endif

#include "AppUtils.h"

using namespace ci;

namespace mndl { namespace app {

#if not defined( CINDER_MAC )
int showMessageBox( const std::string &message, const std::string &title )
{
#if defined( CINDER_MSW )
	return MessageBox( NULL, (LPCWSTR)( toUtf16( message ).c_str() ),
			(LPCWSTR)( toUtf16( title ).c_str() ), MB_OK );
#else
	return 0;
#endif
}
#endif

} } // namespace mndl::app

