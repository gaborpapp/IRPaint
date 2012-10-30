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
#include "cinder/Vector.h"

#include "AntTweakBar.h"

#include "Resources.h"

#include "BlobTracker.h"
#include "PParams.h"
#include "Stroke.h"
#include "Utils.h"

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
		Surface mBrushesMap;
		Surface mBrushesStencil; //<< area of the brushes for more accurate brush selection
		Surface mColorsMap;

		gl::Fbo mDrawing;
		void clearDrawing();

		void loadImages();

		ColorA mBrushColor; //<< current brush color
		int32_t mBrushIndex; //<< current brush index
#define MAX_BRUSHES 5
#define BRUSH_ERASER 5 // id of the eraser, has to be handled separately
		float mBrushThickness[ MAX_BRUSHES + 1 ]; //< thickness of brushes, index range is 1-5
};

IRPaint::IRPaint() :
	mBrushColor( ColorA::black() ),
	mBrushIndex( 3 )
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
}

/** Selects color and brush size from the toolbar from the location of \a pos.
 *  Called on blob enter or mouse down.
 *  \param pos position in window coordinates **/
void IRPaint::selectTools( const Vec2f &pos, const Area &area )
{
	// remember these for switching after the eraser
	static ColorA storedBrushColor;
	static int32_t storedBrushIndex;

	ColorA lastBrushColor = mBrushColor;
	int32_t lastBrushIndex = mBrushIndex;

	Vec2f mapPos = mMapMapping.map( pos );
	Vec2i mapPosi( (int)mapPos.x, (int)mapPos.y );

	ColorA8u colorSelect = mColorsMap.getPixel( mapPosi );
	if ( colorSelect.a )
	{
		mBrushColor = colorSelect;
	}

	ColorA8u brushSelect = mBrushesMap.getPixel( mapPosi );
	if ( brushSelect.a )
	{
		mBrushIndex = ( brushSelect.r & 0x04 ) |
					  ( brushSelect.g & 0x02 ) |
					  ( brushSelect.b & 0x01 );
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

		// calculate histogram of colors
		map< uint32_t, int > hist;
		Surface::Iter iter = mBrushesMap.getIter( areaMap );
		while ( iter.line() )
		{
			while ( iter.pixel() )
			{
				uint8_t a = iter.a();
				uint32_t c = ( iter.r() << 16 ) |
							 ( iter.g() << 8 ) |
							 ( iter.b() );
				if ( a )
					hist[ c ]++;
			}
		}

		// find maximum in histogram
		uint32_t maxColor;
		int maxCount = 0;
		for ( map< uint32_t, int >::iterator it = hist.begin();
				it != hist.end(); ++it )
		{
			if ( it->second > maxCount )
			{
				maxCount = it->second;
				maxColor = it->first;
			}
		}

		// calculate the index from the color
		int32_t index = ( ( maxColor >> 16 ) & 0x04 ) |
						( ( maxColor >> 8 ) & 0x02 ) |
						( maxColor & 0x01 );
		if ( ( index > 0 ) && ( index <= MAX_BRUSHES ) )
		{
			mBrushIndex = index;
		}
	}

	// brush or color change
	if ( ( lastBrushIndex != mBrushIndex ) ||
		 ( lastBrushColor != mBrushColor ) )
	{
		// remember brush/color if switched to eraser
		if ( ( mBrushIndex == BRUSH_ERASER ) &&
			 ( lastBrushIndex != BRUSH_ERASER ) )
		{
			storedBrushIndex = lastBrushIndex;
			storedBrushColor = lastBrushColor;
		}

		// restore brush/color if the last brush was the eraser
		if ( lastBrushIndex == BRUSH_ERASER )
		{
			// brush change - restore color
			if ( mBrushIndex != BRUSH_ERASER )
			{
				mBrushColor = storedBrushColor;
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

	beginStroke( event.getId(), pos );
}

void IRPaint::blobsMoved( mndl::BlobEvent event )
{
	Vec2f pos = mCalibratorRef->map( event.getPos() );
	pos = mCoordMapping.map( pos );

	updateStroke( event.getId(), pos );
}

void IRPaint::blobsEnded( mndl::BlobEvent event )
{
	endStroke( event.getId() );
}

void IRPaint::setup()
{
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
	mParams.addButton( "Reset", std::bind( &IRPaint::clearDrawing, this ), " key=SPACE " );

	mParams.addText( "Brushes" );
	mParams.addPersistentParam( "1st", &mBrushThickness[ 1 ], 10, " min=1 max=200" );
	mParams.addPersistentParam( "2nd", &mBrushThickness[ 2 ], 20, " min=1 max=200" );
	mParams.addPersistentParam( "3rd", &mBrushThickness[ 3 ], 30, " min=1 max=200" );
	mParams.addPersistentParam( "4th", &mBrushThickness[ 4 ], 50, " min=1 max=200" );
	mParams.addPersistentParam( "Eraser", &mBrushThickness[ 5 ], 80, " min=1 max=200" );
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
	mMixerShader.unbind();

	loadImages();

	mDrawing = gl::Fbo( mBackground.getWidth(), mBackground.getHeight() );
	clearDrawing();

	mTracker.setup();
	mCalibratorRef = mTracker.getCalibrator();

	mTracker.registerBlobsCallbacks< IRPaint >( &IRPaint::blobsBegan,
												&IRPaint::blobsMoved,
												&IRPaint::blobsEnded, this );

	setFrameRate( 60 );

	showAllParams( false );
}

void IRPaint::clearDrawing()
{
	mDrawing.bindFramebuffer();
	gl::clear( ColorA( 1, 1, 1, 0 ) );
	mDrawing.unbindFramebuffer();
}

void IRPaint::loadImages()
{
	// all images are the same size (1024x768)
	mBackground = gl::Texture( loadImage( loadResource( RES_BACKGROUND ) ) );
	mColorsMap = loadImage( loadResource( RES_MAP_COLORS ) );
	mBrushesMap = loadImage( loadResource( RES_MAP_BRUSHES ) );
	mAreaStencil = gl::Texture( loadImage( loadResource( RES_AREA_STENCIL ) ) );
	mBrushesStencil = loadImage( loadResource( RES_BRUSHES_STENCIL ) );

	mMapMapping = RectMapping( getWindowBounds(), mBrushesMap.getBounds() );
}

void IRPaint::shutdown()
{
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
	gl::drawSolidRect( getWindowBounds() );
	mMixerShader.unbind();

	mTracker.draw();

	params::PInterfaceGl::draw();
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
	selectTools( pos, Area( pos - Vec2i( 1, 1 ), pos + Vec2i( 1, 1 ) ) );
	beginStroke( 1, pos );
}

void IRPaint::mouseDrag( MouseEvent event )
{
	updateStroke( 1, event.getPos() );
}

void IRPaint::mouseUp( MouseEvent event )
{
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

//CINDER_APP_BASIC( IRPaint, RendererGl( RendererGl::AA_NONE ) )
CINDER_APP_BASIC( IRPaint, RendererGl )

