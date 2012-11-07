#include "cinder/app/App.h"
#if defined( CINDER_COCOA )
	#if defined( CINDER_MAC )
		#import "cinder/app/CinderView.h"
		#import <Cocoa/Cocoa.h>
	#endif
	#include "cinder/cocoa/CinderCocoa.h"
#endif

#include "AppUtils.h"

using namespace ci;

namespace mndl { namespace app {

#if defined( CINDER_MAC )
// from https://github.com/fieldOfView/Cinder/commit/a5e3ca25032b8318834042a800c295cc050c7480
int showMessageBox( const std::string &message, const std::string &title )
{
    bool wasFullScreen = ci::app::isFullScreen();
	ci::app::setFullScreen( false );

	NSAlert *cinderAlert = [[NSAlert alloc] init];
	[cinderAlert addButtonWithTitle: @"OK"];
	[cinderAlert setMessageText: [NSString stringWithUTF8String:title.c_str()] ];
	[cinderAlert setInformativeText: [NSString stringWithUTF8String:message.c_str()] ];
	[cinderAlert setAlertStyle: NSWarningAlertStyle];

	int resultCode = [cinderAlert runModal];
    [cinderAlert release];

	ci::app::setFullScreen( wasFullScreen );
	ci::app::restoreWindowContext();

	return resultCode;
}
#endif

} } // namespace mndl::app

