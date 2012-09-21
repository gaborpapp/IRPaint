#include "cinder/app/App.h"
#include "cinder/Area.h"

#include "CinderOpenCV.h"

#include "BlobTracker.h"

using namespace ci;
using namespace std;

namespace mndl {

void BlobTracker::setup()
{
	// capture

	// list out the capture devices
	vector< Capture::DeviceRef > devices( Capture::getDevices() );
	vector< string > deviceNames;

    for ( vector< Capture::DeviceRef >::const_iterator deviceIt = devices.begin();
			deviceIt != devices.end(); ++deviceIt )
	{
        Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName(); // + " " + device->getUniqueId();

        try
		{
            if ( device->checkAvailable() )
			{
                mCaptures.push_back( Capture( CAPTURE_WIDTH, CAPTURE_HEIGHT,
							device ) );
				deviceNames.push_back( deviceName );
            }
            else
			{
                mCaptures.push_back( Capture() );
				deviceNames.push_back( deviceName + " not available" );
			}
        }
        catch ( CaptureExc & )
		{
			app::console() << "Unable to initialize device: " << device->getName() <<
 endl;
        }
	}

	if ( deviceNames.empty() )
	{
		deviceNames.push_back( "Camera not available" );
		mCaptures.push_back( Capture() );
	}

	// normalized camera coordinates mapping
	mNormMapping = RectMapping( Rectf( 0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT ),
								Rectf( 0, 0, 1, 1 ) );

	// params
	mParams = params::PInterfaceGl( "Tracker", Vec2i( 350, 550 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Capture", deviceNames, &mCurrentCapture, 0 );
	if ( mCurrentCapture >= mCaptures.size() )
		mCurrentCapture = 0;
	mParams.addSeparator();

	mParams.addPersistentParam( "Draw capture", &mDrawCapture, true, "key='c'" );

	mParams.addPersistentParam( "Flip", &mFlip, true );

	mParams.addPersistentParam( "Threshold", &mThreshold, 170, "min=0 max=255");
	mParams.addPersistentParam( "Blur size", &mBlurSize, 10, "min=1 max=15" );
	mParams.addPersistentParam( "Min area", &mMinArea, 0.04f, "min=0.0 max=1.0 step=0.005" );
	mParams.addPersistentParam( "Max area", &mMaxArea, 0.2f, "min=0.0 max=1.0 step=0.005" );
}

void BlobTracker::update()
{
	static int lastCapture = -1;

	if ( lastCapture != mCurrentCapture )
	{
		if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
			mCaptures[ lastCapture ].stop();

		if ( mCaptures[ mCurrentCapture ] )
			mCaptures[ mCurrentCapture ].start();

		mCapture = mCaptures[ mCurrentCapture ];
		lastCapture = mCurrentCapture;
	}

	if ( mCapture && mCapture.checkNewFrame() )
	{
		Channel8u captChan8( Channel8u( mCapture.getSurface() ) );

		// opencv
		cv::Mat input( toOcv( captChan8 ) );
		if ( mFlip )
			cv::flip( input, input, 1 );
		cv::Mat blurred, thresholded;

		cv::blur( input, blurred, cv::Size( mBlurSize, mBlurSize ) );
		cv::threshold( blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY );

		mTextureOrig = gl::Texture( fromOcv( input ) );
		mTextureBlurred = gl::Texture( fromOcv( blurred ) );
		mTextureThresholded = gl::Texture( fromOcv( thresholded ) );

		vector< vector< cv::Point > > contours;
		cv::findContours( thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

		float surfArea = captChan8.getWidth() * captChan8.getHeight();
		float minAreaLimit = surfArea * mMinArea;
		float maxAreaLimit = surfArea * mMaxArea;

		mBlobs.clear();
		for ( vector< vector< cv::Point > >::iterator cit = contours.begin(); cit < contours.end(); ++cit )
		{
			Blob b;
			cv::Mat pmat = cv::Mat( *cit );
			cv::Rect cvRect = cv::boundingRect( pmat );
			b.bbox = Rectf( cvRect.x, cvRect.y,
							cvRect.x + cvRect.width, cvRect.y + cvRect.height );
			float area = b.bbox.calcArea();
			if ( ( minAreaLimit <= area ) && ( area < maxAreaLimit ) )
			{
				cv::Moments m = cv::moments( pmat );
				b.centroid = Vec2f( m.m10 / m.m00, m.m01 / m.m00 );

				b.bbox = mNormMapping.map( b.bbox );
				b.centroid = mNormMapping.map( b.centroid );
				mBlobs.push_back( b );
			}
		}
	}
}

size_t BlobTracker::getBlobNum() const
{
	return mBlobs.size();
}

Rectf BlobTracker::getBlobBoundingRect( size_t i ) const
{
	return mBlobs[ i ].bbox;
}

Vec2f BlobTracker::getBlobCentroid( size_t i ) const
{
	return mBlobs[ i ].centroid;
}

void BlobTracker::draw()
{
	gl::enableAlphaBlending();
	gl::color( ColorA::gray( 1.f, .5f ) );
	if ( mDrawCapture && mTextureOrig )
	{
		Area outputArea = Area::proportionalFit( mTextureOrig.getBounds(),
				app::getWindowBounds(), true, true );

		Rectf halfRect = Rectf( outputArea ) / 2.f;
		gl::draw( mTextureOrig, halfRect );
		gl::draw( mTextureBlurred, halfRect + Vec2f( halfRect.getWidth(), 0.f ) );
		gl::draw( mTextureThresholded, halfRect + Vec2f( 0.f, halfRect.getHeight() ) );

		RectMapping blobMapping( Rectf( 0, 0, 1, 1 ), halfRect );
		for ( size_t i = 0; i < mBlobs.size(); ++i )
		{
			Rectf bbox = getBlobBoundingRect( i );
			gl::color( ColorA( 1, 0, 0, .5 ) );
			gl::drawStrokedRect( blobMapping.map( getBlobBoundingRect( i ) ) );
			gl::drawSolidCircle( blobMapping.map( getBlobCentroid( i ) ), 2 );
		}
		gl::color( Color::white() );
	}
	gl::disableAlphaBlending();
}

void BlobTracker::shutdown()
{
	if ( mCapture )
		mCapture.stop();
}

} // namspace mndl

