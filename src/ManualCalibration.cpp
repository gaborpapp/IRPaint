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

#include "cinder/app/App.h"

#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/Xml.h"

#include "BlobTracker.h"
#include "ManualCalibration.h"

using namespace ci;
using namespace std;

namespace mndl {

ManualCalibration::ManualCalibration( BlobTracker *bt ) :
	mBlobTrackerRef( bt ), mIsCalibrating( false ),
	mIsDebugging( false )
{
	mParams = params::PInterfaceGl( "Calibration", Vec2i( 350, 550 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addButton( "Calibrate", std::bind( &ManualCalibration::toggleCalibrationCB, this ) );
	mParams.addParam( "Debug", &mIsDebugging );
	mParams.addPersistentParam( "Grid width", &mCalibrationGridSize.x, 2, "min=2 max=16" );
	mParams.addPersistentParam( "Grid height", &mCalibrationGridSize.y, 2, "min=2 max=16" );

	resetGrid();
	load();
}

ManualCalibration::~ManualCalibration()
{
	save();
}

void ManualCalibration::startCalibration()
{
	if ( mIsCalibrating )
		toggleCalibrationCB();
	toggleCalibrationCB();
}

void ManualCalibration::stopCalibration()
{
	if ( mIsCalibrating )
		toggleCalibrationCB();
}

void ManualCalibration::resetGrid()
{
	mCalibrationGrid.clear();
	float stepX = 1.f / (float)( mCalibrationGridSize.x - 1 );
	float stepY = 1.f / (float)( mCalibrationGridSize.y - 1 );
	for ( int y = 0; y < mCalibrationGridSize.y; y++ )
	{
		for ( int x = 0; x < mCalibrationGridSize.x; x++ )
		{
			mCalibrationGrid.push_back( Vec2f( stepX * x, stepY * y ) );
		}
	}
	mCameraCalibrationGrid = mCalibrationGrid;
	setupTriangleGrid();
}

void ManualCalibration::setupTriangleGrid()
{
	mTriangleGrid.clear();
	mCameraTriangleGrid.clear();

	Vec2f p0, p1, p2;
	for ( size_t y = 0; y < mCalibrationGridSize.y - 1; y++ )
	{
		for ( size_t x = 0; x < mCalibrationGridSize.x - 1; x++ )
		{
			size_t offset = x + y * mCalibrationGridSize.x;
			p0 = mCalibrationGrid[ offset ];
			p1 = mCalibrationGrid[ offset + mCalibrationGridSize.x ];
			p2 = mCalibrationGrid[ offset + mCalibrationGridSize.x  + 1];
			mTriangleGrid.push_back( Trianglef( p0, p1, p2 ) );

			p0 = mCalibrationGrid[ offset ];
			p1 = mCalibrationGrid[ offset + mCalibrationGridSize.x  + 1];
			p2 = mCalibrationGrid[ offset + 1 ];
			mTriangleGrid.push_back( Trianglef( p0, p1, p2 ) );

			p0 = mCameraCalibrationGrid[ offset ];
			p1 = mCameraCalibrationGrid[ offset + mCalibrationGridSize.x ];
			p2 = mCameraCalibrationGrid[ offset + mCalibrationGridSize.x  + 1];
			mCameraTriangleGrid.push_back( Trianglef( p0, p1, p2 ) );

			p0 = mCameraCalibrationGrid[ offset ];
			p1 = mCameraCalibrationGrid[ offset + mCalibrationGridSize.x  + 1];
			p2 = mCameraCalibrationGrid[ offset + 1 ];
			mCameraTriangleGrid.push_back( Trianglef( p0, p1, p2 ) );
		}
	}
}

void ManualCalibration::toggleCalibrationCB()
{
	static boost::signals2::connection sBeganCB;

	if ( mIsCalibrating )
	{
		mParams.setOptions( "Calibrate", "label=`Calibrate`" );
		mBlobTrackerRef->unregisterBlobsCallback( sBeganCB );

		// calibration is not finished, reset camera grid to default
		if ( mCalibrationGridIndex < mCalibrationGrid.size() )
		{
			resetGrid();
		}
	}
	else
	{
		resetGrid();

		mParams.setOptions( "Calibrate", "label=`Stop calibration`" );
		sBeganCB = mBlobTrackerRef->registerBlobsBegan< ManualCalibration >( &ManualCalibration::blobsBegan, this );
		mCalibrationGridIndex = 0;
		mLastCalibrationIndexReceived = -1;
		mCalibrationId = 0;
		mCameraCalibrationGrid.clear();
	}
	mIsCalibrating = !mIsCalibrating;
}

void ManualCalibration::update()
{
	if ( !mIsCalibrating )
		return;

	if ( mLastCalibrationIndexReceived == mCalibrationGridIndex )
	{
		mCameraCalibrationGrid.push_back( mCalibrationPos );
		mCalibrationGridIndex++;
		// calibration finished?
		if ( mCalibrationGridIndex >= mCalibrationGrid.size() )
		{
			toggleCalibrationCB();
			setupTriangleGrid();
		}
	}
}

Vec2f ManualCalibration::map( const ci::Vec2f &p )
{
	// TODO: optimize this with a lookup table maybe?

	for ( vector< Trianglef >::const_iterator it = mCameraTriangleGrid.begin(),
			cit = mTriangleGrid.begin();
			it != mCameraTriangleGrid.end(); ++it, ++cit )
	{
		if ( it->contains( p ) )
		{
			Vec3f bary = it->toBarycentric( p );
			return cit->fromBarycentric( bary );
		}
	}

	// point is outside of the grid
	// map position according to first triangle (any triangle would do it)
	Vec3f bary = mCameraTriangleGrid[ 0 ].toBarycentric( p );
	return mTriangleGrid[ 0 ].fromBarycentric( bary );
}

void ManualCalibration::draw()
{
	RectMapping calibMapping( Rectf( 0, 0, 1, 1 ), app::getWindowBounds() );

	if ( mIsCalibrating )
	{
		float radius = app::getWindowHeight() / ( 2. * mCalibrationGrid.size() );
		for ( size_t i = 0; i < mCalibrationGrid.size(); i++ )
		{
			if ( i < mCalibrationGridIndex )
				gl::color( Color( 0, 1, 0 ) );
			else
				if ( i == mCalibrationGridIndex )
					gl::color( Color( 1, .8, .1 ) );
				else
					gl::color( Color::gray( .8 ) );
			gl::drawSolidCircle( calibMapping.map( mCalibrationGrid[ i ] ), radius );
		}
	}
	else
	if ( mIsDebugging )
	{
		bool hasBlobs = mBlobTrackerRef->getBlobNum() > 0;
		Vec2f pos;
		if ( hasBlobs )
			pos = mBlobTrackerRef->getBlobCentroid( 0 );

		for ( vector< Trianglef >::const_iterator it = mCameraTriangleGrid.begin(),
			    cit = mTriangleGrid.begin();
				it != mCameraTriangleGrid.end(); ++it, ++cit )
		{
			if ( hasBlobs && it->contains( pos ) )
			{
				gl::color( ColorA( 1, 0, 0, .5 ) );
				gl::drawSolidTriangle( calibMapping.map( it->a() ),
						calibMapping.map( it->b() ),
						calibMapping.map( it->c() ) );

				Vec3f bary = it->toBarycentric( pos );
				Vec2f p = cit->fromBarycentric( bary );
				gl::color( ColorA( 1, .6, .1, .5 ) );
				gl::drawSolidCircle( calibMapping.map( p ), 10 );
			}
			else
			{
				gl::color( Color::white() );
				gl::drawStrokedTriangle( calibMapping.map( it->a() ),
						calibMapping.map( it->b() ),
						calibMapping.map( it->c() ) );
			}
		}
	}
	gl::color( Color::white() );
}

void ManualCalibration::blobsBegan( BlobEvent event )
{
	if ( ( mLastCalibrationIndexReceived < (int)mCalibrationGridIndex ) &&
		 ( mCalibrationId != event.getId() ) )
	{
		mLastCalibrationIndexReceived = mCalibrationGridIndex;
		mCalibrationPos = event.getPos();
		mCalibrationId = event.getId();
	}
}

void ManualCalibration::load( const fs::path &fname )
{
	fs::path path = fname;

	if ( path.empty() )
	{
		path = app::getAssetPath( "calibration.xml" );
		if ( fname.empty() )
		{
#if defined( CINDER_MAC )
			fs::path assetPath( app::App::getResourcePath() / "assets" );
#else
			fs::path assetPath( app::getAppPath() / "assets" );
#endif
			createDirectories( assetPath );
			path = assetPath / "calibration.xml" ;
		}
	}
	mConfigFile = path;

	if ( !fs::exists( path ) )
		return;

	XmlTree config( loadFile( path ) );

	XmlTree grid = config.getChild( "calibration/grid" );
	mCalibrationGridSize.x = grid.getAttributeValue< int >( "width" );
	mCalibrationGridSize.y = grid.getAttributeValue< int >( "height" );

	resetGrid();
	mCameraCalibrationGrid.clear();

	for( XmlTree::Iter pit = config.begin( "calibration/point");
			pit != config.end(); ++pit )
	{
		Vec2f point( pit->getAttributeValue< float >( "x" ),
					 pit->getAttributeValue< float >( "y" ) );
		mCameraCalibrationGrid.push_back( point );
	}
	setupTriangleGrid();
}

void ManualCalibration::save()
{
	XmlTree config( "calibration", "" );

	XmlTree grid( "grid", "" );
	grid.setAttribute( "width", mCalibrationGridSize.x );
	grid.setAttribute( "height", mCalibrationGridSize.y );

	config.push_back( grid );

	for ( vector< Vec2f >::const_iterator it = mCameraCalibrationGrid.begin();
			it != mCameraCalibrationGrid.end(); ++it )
	{
		XmlTree t = XmlTree( "point", "" );
		t.setAttribute( "x", it->x );
		t.setAttribute( "y", it->y );
		config.push_back( t );
	}
	config.write( writeFile( mConfigFile ) );
}

} // namespace mndl

