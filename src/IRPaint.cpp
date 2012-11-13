/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <map>
#include <vector>

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Cinder.h"
#include "cinder/Rect.h"
#include "cinder/Timeline.h"
#include "cinder/Vector.h"

#include "AntTweakBar.h"

#include "Resources.h"

#include "AppUtils.h"
#include "BlobTracker.h"
#include "PParams.h"
#include "Stroke.h"
#include "Utils.h"
#include "TextureMenu.h"
#include "License.h"

#if defined( CINDER_MSW )
#include "resource.h"
#endif

using namespace ci;
using namespace ci::app;
using namespace std;

class IRPaint : public AppBasic
{
	public:
		IRPaint();

		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );
		void resize( ResizeEvent event );

		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void mouseUp( MouseEvent event );

		void update();
		void draw();

	private:
		params::PInterfaceGl mParams;
		void showAllParams( bool visible );

		Anim< bool > mDrawSplash; // true if the splash screen should be drawn
		float mSplashDuration;

		float mFps;
		mndl::BlobTracker mTracker;
		shared_ptr< mndl::ManualCalibration > mCalibratorRef;

		map< int32_t, Stroke > mStrokes;

		void beginStroke( int32_t id, const Vec2f &pos );
		void updateStroke( int32_t id, const Vec2f &pos );
		void endStroke( int32_t id );

		//! Mapping from normalized coordinates to window size
		RectMapping mCoordMapping;
		//! Mapping from window coordinates to brush and color map size (1024x768)
		RectMapping mMapMapping;

		void blobsBegan( mndl::BlobEvent event );
		void blobsMoved( mndl::BlobEvent event );
		void blobsEnded( mndl::BlobEvent event );

		void selectTools( const Vec2f &pos, const Area &area );
		gl::GlslProg mMixerShader;

		// textures, surfaces
		gl::Texture mBackground;
		gl::Texture mAreaStencil;
		gl::Texture mSplashScreen;
		Surface mBrushesMap;
		Surface mBrushesStencil; //<< area of the brushes for more accurate brush selection
		Surface mColorsMap;
		Surface mColorPalette; //<< list of colors

		vector< gl::Texture > mBrushesGlow; //<< glow textures for the brushes
		vector< gl::Texture > mColorsGlow; //<< glow textures for the colors

		Area mDrawingArea; //<< bounding area of the drawing
		gl::Fbo mDrawing;
		void clearDrawing();

		void loadImages();
		bool checkLicense();

		ColorA mBrushColor; //<< current brush color
		int32_t mBrushIndex; //<< current brush index
		int32_t mColorIndex; //<< current color index
#define MAX_BRUSHES 5
#define BRUSH_ERASER 4 // id of the eraser, has to be handled separately
#define MENU_ID 5 // id of the menu on the brush map
		float mBrushThickness[ MAX_BRUSHES ]; //< thickness of brushes, index range is 0-4

		void setupMenu();
		mndl::gl::TextureMenu mMenu;

		// screenshots
		fs::path mScreenshotFolder;
		void saveScreenshot();
		void threadedScreenshot( Surface snapshot );
		thread mScreenshotThread;
};

IRPaint::IRPaint() :
	mBrushColor( ColorA::black() ),
	mBrushIndex( 2 ),
	mColorIndex( 7 )
{}

void IRPaint::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void IRPaint::resize( ResizeEvent event )
{
	mCoordMapping = RectMapping( Rectf( 0, 0, 1, 1 ),
			Rectf( Vec2f( 0, 0 ), event.getSize() ) );
	mMapMapping = RectMapping( Rectf( Vec2f::zero(), event.getSize() ),
							   mBrushesMap.getBounds() );
	mMenu.setPosition( ( event.getSize() - mMenu.getSize() ) / 2 );
}

/** Selects color and brush size from the toolbar from the location of \a pos.
 *  Called on blob enter or mouse down.
 *  \param pos position in window coordinates **/
void IRPaint::selectTools( const Vec2f &pos, const Area &area )
{
	// remember these for switching after the eraser
	static int32_t storedBrushIndex;
	static int32_t storedColorIndex;

	int32_t lastBrushIndex = mBrushIndex;
	int32_t lastColorIndex = mColorIndex;

	Vec2f mapPos = mMapMapping.map( pos );
	Vec2i mapPosi( (int)mapPos.x, (int)mapPos.y );

	ColorA8u colorSelect = mColorsMap.getPixel( mapPosi );
	if ( colorSelect.a )
	{
		int32_t index = colorSelect.b;
		if ( index < mColorPalette.getWidth() )
		{
			mColorIndex = colorSelect.b;
			mBrushColor = mColorPalette.getPixel( Vec2i( mColorIndex, 0 ) );
		}
	}

	ColorA8u brushSelect = mBrushesMap.getPixel( mapPosi );
	if ( brushSelect.a )
	{
		int32_t index = brushSelect.b;
		if ( index < MAX_BRUSHES )
		{
			mBrushIndex = index;
		}
		else
		if ( index == MENU_ID )
		{
			// show/hide menu
			mMenu.show( !mMenu.isVisible() );
		}
	}
	else
	if ( mBrushesStencil.getPixel( mapPosi ).a ) // more accurate brush selection
	{
		// map area
		Vec2i ul = area.getUL();
		Vec2i lr = area.getLR();
		Vec2f ulM = mMapMapping.map( ul );
		Vec2f lrM = mMapMapping.map( lr );
		Area areaMap( (int32_t)ulM.x, (int32_t)ulM.y,
					  (int32_t)lrM.x, (int32_t)lrM.y );

		// calculate histogram of brush indices
		map< int32_t, int > hist;
		Surface::Iter iter = mBrushesMap.getIter( areaMap );
		while ( iter.line() )
		{
			while ( iter.pixel() )
			{
				uint8_t a = iter.a();
				int32_t c = iter.b();
				if ( a )
					hist[ c ]++;
			}
		}

		// find maximum in histogram
		int32_t maxBrushIndex = 0;
		int maxCount = 0;
		for ( map< int32_t, int >::iterator it = hist.begin();
				it != hist.end(); ++it )
		{
			if ( it->second > maxCount )
			{
				maxCount = it->second;
				maxBrushIndex = it->first;
			}
		}

		// calculate the index from the color
		int32_t index = maxBrushIndex;
		if ( index < MAX_BRUSHES )
		{
			mBrushIndex = index;
		}
	}

	// brush or color change
	if ( ( lastBrushIndex != mBrushIndex ) ||
		 ( lastColorIndex != mColorIndex ) )
	{
		// remember brush/color if switched to eraser
		if ( ( mBrushIndex == BRUSH_ERASER ) &&
			 ( lastBrushIndex != BRUSH_ERASER ) )
		{
			storedBrushIndex = lastBrushIndex;
			storedColorIndex = lastColorIndex;
		}

		// restore brush/color if the last brush was the eraser
		if ( lastBrushIndex == BRUSH_ERASER )
		{
			// brush change - restore color
			if ( mBrushIndex != BRUSH_ERASER )
			{
				mColorIndex = storedColorIndex;
				mBrushColor = mColorPalette.getPixel( Vec2i( mColorIndex, 0 ) );
			}
			else // color change - restore brush
			{
				mBrushIndex = storedBrushIndex;
			}
		}
	}
}

void IRPaint::beginStroke( int32_t id, const Vec2f &pos )
{
	mStrokes[ id ] = Stroke();
	Vec2f drawingPos = mMapMapping.map( pos );
	mStrokes[ id ].resize( ResizeEvent( mDrawing.getSize() ) );
	mStrokes[ id ].setColor( mBrushColor );
	mStrokes[ id ].setThickness( mBrushThickness[ mBrushIndex ] );

	mStrokes[ id ].update( drawingPos );
}

void IRPaint::updateStroke( int32_t id, const Vec2f &pos )
{
	Vec2f drawingPos = mMapMapping.map( pos );
	mStrokes[ id ].update( drawingPos );
}

void IRPaint::endStroke( int32_t id )
{
	map< int32_t, Stroke >::iterator it;
	it = mStrokes.find( id );
	mStrokes.erase( id );
}


void IRPaint::blobsBegan( mndl::BlobEvent event )
{
	if ( mCalibratorRef->isCalibrating() )
		return;

	Vec2f pos = mCalibratorRef->map( event.getPos() );
	pos = mCoordMapping.map( pos );

	// map bounding box
	Rectf bbox = event.getBoundingBox();
	Vec2f ul = mCalibratorRef->map( bbox.getUpperLeft() );
	Vec2f lr = mCalibratorRef->map( bbox.getLowerRight() );
	ul = mCoordMapping.map( ul );
	lr = mCoordMapping.map( lr );
	Area area = Area( int32_t( ul.x ), int32_t( ul.y ),
					  int32_t( lr.x ), int32_t( lr.y ) );

	selectTools( pos, area );

	if ( mMenu.isVisible() )
		mMenu.processClick( pos );
	else
		beginStroke( event.getId(), pos );
}

void IRPaint::blobsMoved( mndl::BlobEvent event )
{
	if ( mCalibratorRef->isCalibrating() )
		return;

	Vec2f pos = mCalibratorRef->map( event.getPos() );
	pos = mCoordMapping.map( pos );

	if ( !mMenu.isVisible() )
		updateStroke( event.getId(), pos );
}

void IRPaint::blobsEnded( mndl::BlobEvent event )
{
	if ( mCalibratorRef->isCalibrating() )
		return;

	if ( !mMenu.isVisible() )
		endStroke( event.getId() );
}

void IRPaint::setup()
{
#if defined( CINDER_MSW )
	setIcon( IDI_ICON1 );
#endif

	if ( !checkLicense() )
	{
		quit();
	}

	gl::disableVerticalSync();

	// params
	fs::path paramsXml( getAssetPath( "params.xml" ));
	if ( paramsXml.empty() )
	{
#if defined( CINDER_MAC )
		fs::path assetPath( getResourcePath() / "assets" );
#else
		fs::path assetPath( getAppPath() / "assets" );
#endif
		createDirectories( assetPath );
		paramsXml = assetPath / "params.xml" ;
	}
	params::PInterfaceGl::load( paramsXml );

	mParams = params::PInterfaceGl( "Parameters", Vec2i( 350, 550 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addButton( "Reset", std::bind( &IRPaint::clearDrawing, this ), "key=SPACE" );
	mParams.addButton( "Screenshot", std::bind( &IRPaint::saveScreenshot, this ), "key=w" );
	mParams.addPersistentParam( "Splash duration", &mSplashDuration, 5.f, "min=1 max=30 step=.5" );

	mParams.addText( "Brushes" );
	mParams.addPersistentParam( "1st", &mBrushThickness[ 0 ], 10, "min=1 max=200" );
	mParams.addPersistentParam( "2nd", &mBrushThickness[ 1 ], 20, "min=1 max=200" );
	mParams.addPersistentParam( "3rd", &mBrushThickness[ 2 ], 30, "min=1 max=200" );
	mParams.addPersistentParam( "4th", &mBrushThickness[ 3 ], 50, "min=1 max=200" );
	mParams.addPersistentParam( "Eraser", &mBrushThickness[ BRUSH_ERASER ], 80, "min=1 max=200" );
	mParams.addText( "Debug" );
	mParams.addParam( "Brush index", &mBrushIndex, "", true );
	mParams.addParam( "Brush color", &mBrushColor, "", true );

	try
	{
		mMixerShader = gl::GlslProg( loadResource( RES_PASSTHROUGH_VERT ),
									 loadResource( RES_MIXER_FRAG ) );
	}
	catch ( const std::exception &exc )
	{
		console() << exc.what() << endl;
		quit();
	}

	mMixerShader.bind();
	mMixerShader.uniform( "drawing", 0 );
	mMixerShader.uniform( "stencil", 1 );
	mMixerShader.uniform( "glow0", 2 );
	mMixerShader.uniform( "glow1", 3 );
	mMixerShader.unbind();

	loadImages();
	setupMenu();

	gl::Fbo::Format format;
	format.enableDepthBuffer( false );
	format.setSamples( 4 );
	mDrawing = gl::Fbo( mBackground.getWidth(), mBackground.getHeight(), format );
	clearDrawing();

	mTracker.setup();
	mCalibratorRef = mTracker.getCalibrator();

	mTracker.registerBlobsCallbacks< IRPaint >( &IRPaint::blobsBegan,
												&IRPaint::blobsMoved,
												&IRPaint::blobsEnded, this );

	// screenshots
	mScreenshotFolder = getAppPath();
#ifdef CINDER_MAC
	mScreenshotFolder /= "..";
#endif
	mScreenshotFolder /= "screenshots";
	fs::create_directory( mScreenshotFolder );

	// splash screen timer
	mDrawSplash = true;
	timeline().apply( &mDrawSplash, false, mSplashDuration );

	setFrameRate( 60 );

	showAllParams( false );
}

void IRPaint::clearDrawing()
{
	mDrawing.bindFramebuffer();
	gl::clear( ColorA( 1, 1, 1, 0 ) );
	mDrawing.unbindFramebuffer();

	mStrokes.clear();

	// hide the menu
	timeline().add( std::bind( &mndl::gl::TextureMenu::hide, &mMenu ), timeline().getCurrentTime() + 1 );
}

void IRPaint::loadImages()
{
	// all images are the same size (1024x768)
	mBackground = gl::Texture( loadImage( loadResource( RES_BACKGROUND ) ) );
	mSplashScreen = gl::Texture( loadImage( loadResource( RES_SPLASHSCREEN ) ) );
	mColorsMap = loadImage( loadResource( RES_MAP_COLORS ) );
	mBrushesMap = loadImage( loadResource( RES_MAP_BRUSHES ) );
	Surface areaStencilSurf = loadImage( loadResource( RES_AREA_STENCIL ) );
	mAreaStencil = gl::Texture( areaStencilSurf );
	mBrushesStencil = loadImage( loadResource( RES_BRUSHES_STENCIL ) );
	mColorPalette = loadImage( loadResource( RES_COLOR_PALETTE ) );

	// brush glow textures
	mBrushesGlow.push_back( loadImage( loadResource( RES_GLOW_BRUSH_0 ) ) );
	mBrushesGlow.push_back( loadImage( loadResource( RES_GLOW_BRUSH_1 ) ) );
	mBrushesGlow.push_back( loadImage( loadResource( RES_GLOW_BRUSH_2 ) ) );
	mBrushesGlow.push_back( loadImage( loadResource( RES_GLOW_BRUSH_3 ) ) );
	mBrushesGlow.push_back( loadImage( loadResource( RES_GLOW_ERASER ) ) );

	// color glow textures
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_0 ) ) );
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_1 ) ) );
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_2 ) ) );
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_3 ) ) );
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_4 ) ) );
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_5 ) ) );
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_6 ) ) );
	mColorsGlow.push_back( loadImage( loadResource( RES_GLOW_COLOR_7 ) ) );

	mMapMapping = RectMapping( getWindowBounds(), mBrushesMap.getBounds() );

	// calculate area bounding box for saving drawing
	Surface::ConstIter it = areaStencilSurf.getIter();
	// FIXME: does not work with RectT< int >
	// Undefined symbol: "cinder::RectT<int>::set(int, int, int, int)"?
	Rectf bounds;
	bool rectInited = false;
	while ( it.line() )
	{
		while ( it.pixel() )
		{
			if ( it.a() )
			{
				Vec2i p = it.getPos();
				if ( rectInited )
				{
					bounds.include( p );
				}
				else
				{
					bounds = Rectf( p, p );
					rectInited = true;
				}
			}
		}
	}
	mDrawingArea = Area( bounds );
}

void IRPaint::setupMenu()
{
	mMenu = mndl::gl::TextureMenu( loadImage( loadResource( RES_MENU_BACKGROUND ) ) );
	mMenu.addButton( loadImage( loadResource( RES_MENU_HU_SAVE_ON ) ),
			loadImage( loadResource( RES_MENU_HU_SAVE_OFF ) ),
			&IRPaint::saveScreenshot, this );
	mMenu.addButton( loadImage( loadResource( RES_MENU_HU_CLEAR_ON ) ),
			loadImage( loadResource( RES_MENU_HU_CLEAR_OFF ) ),
			&IRPaint::clearDrawing, this );
	mMenu.hide();
}

void IRPaint::shutdown()
{
	// wait for screenshot thread to finish
	mScreenshotThread.join();

	params::PInterfaceGl::save();

	mTracker.shutdown();
}

void IRPaint::update()
{
	mFps = getAverageFps();

	mTracker.update();
}

void IRPaint::draw()
{
	if ( mDrawSplash )
	{
		// draw background and drawing
		gl::clear( Color::black() );
		gl::setMatricesWindow( getWindowSize() );
		gl::setViewport( getWindowBounds() );

		Area outputArea = Area::proportionalFit( mSplashScreen.getBounds(),
				getWindowBounds(), true );
		gl::draw( mSplashScreen, outputArea );

		return;
	}

	// draw strokes
	mDrawing.bindFramebuffer();
	gl::setMatricesWindow( mDrawing.getSize(), false );
	gl::setViewport( mDrawing.getBounds() );

	gl::enableAlphaBlending();
	for ( map< int32_t, Stroke >::iterator it = mStrokes.begin();
			it != mStrokes.end(); ++it )
	{
		it->second.draw();
	}

	gl::disableAlphaBlending();
	mDrawing.unbindFramebuffer();

	// draw background and drawing
	gl::clear( Color::black() );
	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	gl::color( Color::white() );
	gl::draw( mBackground, getWindowBounds() );

	gl::enableAlphaBlending();

	gl::color( ColorA::white() );
	mMixerShader.bind();
	mDrawing.getTexture().bind();
	mAreaStencil.bind( 1 );
	mBrushesGlow[ mBrushIndex ].bind( 2 );
	mColorsGlow[ mColorIndex ].bind( 3 );
	gl::drawSolidRect( getWindowBounds() );
	mMixerShader.unbind();

	mMenu.draw();

	mTracker.draw();

	params::PInterfaceGl::draw();
}

void IRPaint::saveScreenshot()
{
	// NOTE: slow because GPU->CPU copy
	Surface snapshot( Surface( mDrawing.getTexture() ).clone( mDrawingArea ) );

	// fill transparent area
	Surface::Iter it = snapshot.getIter();
	while ( it.line() )
	{
		while ( it.pixel() )
		{
			uint8_t a = it.a();
			it.r() = ( a * it.r() + ( 255 - a ) * 255 ) >> 8;
			it.g() = ( a * it.g() + ( 255 - a ) * 255 ) >> 8;
			it.b() = ( a * it.b() + ( 255 - a ) * 255 ) >> 8;
			it.a() = 255;
		}
	}

	mScreenshotThread = thread( &IRPaint::threadedScreenshot, this, snapshot );

	// hide the menu
	timeline().add( std::bind( &mndl::gl::TextureMenu::hide, &mMenu ), timeline().getCurrentTime() + 1 );
}

void IRPaint::threadedScreenshot( Surface snapshot )
{
    string filename = "snap-" + mndl::getTimestamp() + ".png";
    fs::path pngPath( mScreenshotFolder / fs::path( filename ) );

    try
    {
        if ( !pngPath.empty() )
        {
            writeImage( pngPath, snapshot );
        }
    }
    catch ( ... )
    {
        console() << "unable to save image file " << pngPath << endl;
    }
}

// show/hide all bars except help, which is always hidden
void IRPaint::showAllParams( bool visible )
{
    int barCount = TwGetBarCount();

    int32_t visibleInt = visible ? 1 : 0;
    for ( int i = 0; i < barCount; ++i )
    {
        TwBar *bar = TwGetBarByIndex( i );
        TwSetParam( bar, NULL, "visible", TW_PARAM_INT32, 1, &visibleInt );
    }

    TwDefine( "TW_HELP visible=false" );
}

void IRPaint::mouseDown( MouseEvent event )
{
	Vec2i pos = event.getPos();
	if ( mMenu.isVisible() )
		mMenu.processClick( pos );
	else
		beginStroke( 1, pos );

	selectTools( pos, Area( pos - Vec2i( 1, 1 ), pos + Vec2i( 1, 1 ) ) );
}

void IRPaint::mouseDrag( MouseEvent event )
{
	if ( !mMenu.isVisible() )
		updateStroke( 1, event.getPos() );
}

void IRPaint::mouseUp( MouseEvent event )
{
	if ( !mMenu.isVisible() )
		endStroke( 1 );
}

void IRPaint::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			showAllParams( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_SPACE:
			clearDrawing();
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

bool IRPaint::checkLicense()
{
	XmlTree doc = XmlTree( loadResource( RES_LICENSE_XML ));

	mndl::license::License license;
	license.init( doc );
	license.setKey( loadResource( RES_LICENSE_KEY ) );

	if ( !license.process() )
	{
		mndl::app::showMessageBox( "License process failed!", "License problem" );
		return false;
	}

	return true;
}

CINDER_APP_BASIC( IRPaint, RendererGl( RendererGl::AA_NONE ) )

