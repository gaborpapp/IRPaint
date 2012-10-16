#include <boost/assign.hpp>

#include "cinder/app/App.h"
#include "cinder/Area.h"

#include "BlobTracker.h"
#include "CinderOpenCV.h"
#include "Utils.h"

using namespace ci;
using namespace std;

namespace mndl {

void BlobTracker::setup()
{
	// capture

	// list out the capture devices
	vector< Capture::DeviceRef > devices( Capture::getDevices() );

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
				mDeviceNames.push_back( deviceName );
            }
            else
			{
                mCaptures.push_back( Capture() );
				mDeviceNames.push_back( deviceName + " not available" );
			}
        }
        catch ( CaptureExc & )
		{
			app::console() << "Unable to initialize device: " << device->getName() <<
 endl;
        }
	}

	if ( mDeviceNames.empty() )
	{
		mDeviceNames.push_back( "Camera not available" );
		mCaptures.push_back( Capture() );
	}

	// normalized camera coordinates mapping
	mNormMapping = RectMapping( Rectf( 0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT ),
								Rectf( 0, 0, 1, 1 ) );

	mParams = params::PInterfaceGl( "Tracker", Vec2i( 350, 550 ) );
	mParams.addPersistentSizeAndPosition();
	setupGui();
}

void BlobTracker::setupGui()
{
	// params
	params::PInterfaceGl::save();
	mParams.clear();

	vector< string > enumNames = boost::assign::list_of("Recording")("Camera");
	mParams.addPersistentParam( "Source", enumNames, &mSource, SOURCE_CAMERA );

	if ( mSource == SOURCE_CAMERA )
	{
		mParams.addPersistentParam( "Capture", mDeviceNames, &mCurrentCapture, 0 );
		if ( mCurrentCapture >= mCaptures.size() )
			mCurrentCapture = 0;
		mParams.addSeparator();

		mParams.addButton( "Save video", std::bind( &BlobTracker::saveVideoCB, this ) );
	}
	else
	{
		mParams.addSeparator();
	    mParams.addButton( "Play video", std::bind( &BlobTracker::playVideoCB, this ) );
	}

	mParams.addSeparator();

	mParams.addPersistentParam( "Flip", &mFlip, true );

	mParams.addPersistentParam( "Threshold", &mThreshold, 170, "min=0 max=255");
	mParams.addPersistentParam( "Blur size", &mBlurSize, 10, "min=1 max=15" );
	mParams.addPersistentParam( "Min area", &mMinArea, 0.001f, "min=0.0 max=1.0 step=0.001" );
	mParams.addPersistentParam( "Max area", &mMaxArea, 0.2f, "min=0.0 max=1.0 step=0.001" );

	mParams.addSeparator();

	enumNames = boost::assign::list_of("None")("Original")("Blurred")("Thresholded");
	mParams.addPersistentParam( "Draw capture", enumNames, &mDrawCapture, 0, "key='c'" );
}

void BlobTracker::update()
{
	static int lastCapture = -1;
	static int lastSource = -1;

	// stop and start capture devices
	if ( lastCapture != mCurrentCapture )
	{
		if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
			mCaptures[ lastCapture ].stop();

		if ( mCaptures[ mCurrentCapture ] )
			mCaptures[ mCurrentCapture ].start();

		mCapture = mCaptures[ mCurrentCapture ];
		lastCapture = mCurrentCapture;
	}

	// change gui buttons if switched between capture and playback
	if ( lastSource != mSource )
	{
		app::console() << "source: " << mSource << endl;
		setupGui();
		lastSource = mSource;
	}

	// select source
	bool processFrame = false;
	Surface8u inputSurface;

	if ( mSource == SOURCE_CAMERA )
	{
		processFrame = mCapture && mCapture.checkNewFrame();
		inputSurface = mCapture.getSurface();
	}
	else // SOURCE_RECORDING
	{
	}

	// process source image
	if ( processFrame )
	{
		if ( mSavingVideo )
			mMovieWriter.addFrame( inputSurface );

		// opencv
		cv::Mat input( toOcv( Channel8u( inputSurface ) ) );
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

		float surfArea = inputSurface.getWidth() * inputSurface.getHeight();
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
	if ( ( mDrawCapture > 0 ) && mTextureOrig )
	{
		Area outputArea = Area::proportionalFit( mTextureOrig.getBounds(),
				app::getWindowBounds(), true, true );

		Rectf halfRect = Rectf( outputArea ) / 2.f;

		gl::Texture txt;
		switch ( mDrawCapture )
		{
			case DRAW_ORIGINAL:
				txt = mTextureOrig;
				break;

			case DRAW_BLURRED:
				txt = mTextureBlurred;
				break;

			case DRAW_THRESHOLDED:
				txt = mTextureThresholded;
				break;

			default:
				break;
		}

		gl::draw( txt, halfRect );

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

void BlobTracker::playVideoCB()
{
	fs::path moviePath = app::getOpenFilePath();
	if ( !moviePath.empty() )
	{
		try
		{
			mMovie = qtime::MovieSurface( moviePath );
			mMovie.setLoop();
			mMovie.play();
		}
		catch (...)
		{
			app::console() << "Unable to load movie " << moviePath << endl;
			mMovie.reset();
		}
	}
}

void BlobTracker::saveVideoCB()
{
	if ( mSavingVideo )
	{
		mParams.setOptions( "Save video", "label=`Save video`" );
		mMovieWriter.finish();
	}
	else
	{
		mParams.setOptions( "Save video", "label=`Finish saving`" );

		qtime::MovieWriter::Format format;
		format.setCodec( qtime::MovieWriter::CODEC_H264 );
		format.setQuality( 0.5f );
		format.setDefaultDuration( 1. / 25. );

		fs::path appPath = app::getAppPath();
#ifdef CINDER_MAC
		appPath /= "..";
#endif
		mMovieWriter = qtime::MovieWriter( appPath /
				fs::path( "capture-" + mndl::getTimestamp() + ".mov" ),
				CAPTURE_WIDTH, CAPTURE_HEIGHT, format );
	}
	mSavingVideo = !mSavingVideo;
}

void BlobTracker::shutdown()
{
	if ( mCapture )
		mCapture.stop();
}

} // namspace mndl

