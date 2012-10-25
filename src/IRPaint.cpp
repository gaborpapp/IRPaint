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

#include <list>
#include <vector>

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Cinder.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

#include "AntTweakBar.h"

#include "Resources.h"

#include "BlobTracker.h"
#include "PParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class IRPaint : public AppBasic
{
	public:
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

		void updateStrokes();
		void resetStrokes();

		typedef list< Vec2f > Stroke;
		vector< Stroke > mStrokes;
		size_t mCurrentStrokeIdx;
		bool mHasBlobs;

		//! Mapping from normalized coordinates to window size
		RectMapping mCoordMapping;
		//! Mapping from normalized coordinates to brush and color map size (1024x768)
		RectMapping mMapMapping;

		void blobsBegan( mndl::BlobEvent event );
		void blobsMoved( mndl::BlobEvent event );
		void blobsEnded( mndl::BlobEvent event );

		void selectTools( const Vec2f &pos );
		void placeBrush( const Vec2f &pos );

		// textures, surfaces
		gl::Texture mBackground;
		Surface mAreaStencil;
		Surface mBrushesMap;
		Surface mColorsMap;

		void loadImages();

		ColorA mBrushColor; //<< current brush color
		uint32_t mBrushIndex; //<< current color index
};

void IRPaint::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void IRPaint::resize( ResizeEvent event )
{
	mCoordMapping = RectMapping( Rectf( 0, 0, 1, 1 ),
			Rectf( Vec2f( 0, 0 ), event.getSize() ) );
}

/** Selects color and brush size from the toolbar from the location of \a pos.
 *  Called on blob enter or mouse down. **/
void IRPaint::selectTools( const Vec2f &pos )
{
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
}

//! Puts paint on the location of \a pos with the current color and brush size.
void IRPaint::placeBrush( const Vec2f &pos )
{
}

void IRPaint::blobsBegan( mndl::BlobEvent event )
{
	Vec2f pos = mCalibratorRef->map( event.getPos() );
	pos = mCoordMapping.map( pos );

	selectTools( pos );
	//console() << "blob began " << event.getId() << " " << event.getPos() << endl;
}

void IRPaint::blobsMoved( mndl::BlobEvent event )
{
	//console() << "blob moved " << event.getId() << " " << event.getPos() << endl;
}

void IRPaint::blobsEnded( mndl::BlobEvent event )
{
	//console() << "blob ended " << event.getId() << " " << event.getPos() << endl;
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
	mParams.addButton( "Reset", std::bind( &IRPaint::resetStrokes, this ), " key=SPACE " );

	mTracker.setup();
	mCalibratorRef = mTracker.getCalibrator();

	mTracker.registerBlobsCallbacks< IRPaint >( &IRPaint::blobsBegan,
												&IRPaint::blobsMoved,
												&IRPaint::blobsEnded, this );

	loadImages();

	setFrameRate( 60 );

	resetStrokes();

	showAllParams( false );
}

void IRPaint::loadImages()
{
	// all images are the same size (1024x768)
	mBackground = gl::Texture( loadImage( loadResource( RES_BACKGROUND ) ) );
	mColorsMap = loadImage( loadResource( RES_MAP_COLORS ) );
	mBrushesMap = loadImage( loadResource( RES_MAP_BRUSHES ) );
	mAreaStencil = loadImage( loadResource( RES_AREA_STENCIL ) );

	mMapMapping = RectMapping( Rectf( 0, 0, 1, 1 ), mBrushesMap.getBounds() );
}

void IRPaint::resetStrokes()
{
	mStrokes.clear();
	mCurrentStrokeIdx = -1;
	mHasBlobs = false;
}

void IRPaint::shutdown()
{
	params::PInterfaceGl::save();

	mTracker.shutdown();
}

void IRPaint::updateStrokes()
{
	mTracker.update();
	int n = mTracker.getBlobNum();
	if ( n > 0 )
	{
		if ( !mHasBlobs )
		{
			mCurrentStrokeIdx++;
			mStrokes.push_back( Stroke() );
		}

		Vec2f pos = mTracker.getBlobCentroid( 0 );
		pos = mCalibratorRef->map( pos );
		pos = mCoordMapping.map( pos );
		mStrokes[ mCurrentStrokeIdx ].push_back( pos );

		mHasBlobs = true;
	}
	else
	{
		mHasBlobs = false;
	}
}

void IRPaint::update()
{
	mFps = getAverageFps();

	updateStrokes();
}

void IRPaint::draw()
{
	gl::clear( Color::black() );

	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	gl::color( Color::white() );
	gl::draw( mBackground, getWindowBounds() );

	gl::enableAdditiveBlending();
	gl::color( ColorA( 1., .8, .2, .4 ) );
	glLineWidth( 30 );
	for ( vector< Stroke >::const_iterator sIt = mStrokes.begin(); sIt != mStrokes.end(); ++sIt )
	{
		gl::begin( GL_LINE_STRIP );
		for ( list< Vec2f >::const_iterator pIt = sIt->begin(); pIt != sIt->end(); ++pIt )
		{
			gl::vertex( *pIt );
		}
		gl::end();
	}
	glLineWidth( 1 );
	gl::disableAlphaBlending();

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
	Vec2f pos = Vec2f( event.getPos() ) / Vec2f( getWindowSize() );
	selectTools( pos );
}

void IRPaint::mouseDrag( MouseEvent event )
{
}

void IRPaint::mouseUp( MouseEvent event )
{
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
			resetStrokes();
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

