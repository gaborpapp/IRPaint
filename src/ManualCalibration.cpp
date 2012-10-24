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
	mBlobTrackerRef( bt ), mIsCalibrating( false )
{
	mParams = params::PInterfaceGl( "Calibration", Vec2i( 350, 550 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addButton( "Calibrate", std::bind( &ManualCalibration::toggleCalibrationCB, this ) );
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
		if ( mCalibrationGridIndex >= mCalibrationGrid.size() )
		{
			app::console() << "camera calibration: " << endl;
			for ( vector< Vec2f >::const_iterator it = mCameraCalibrationGrid.begin();
					it != mCameraCalibrationGrid.end(); ++it )
			{
				app::console() << " " << *it;
			}
			app::console() << endl;
			toggleCalibrationCB();
		}
	}
}

void ManualCalibration::draw()
{
	if ( !mIsCalibrating )
		return;

	RectMapping calibMapping( Rectf( 0, 0, 1, 1 ), app::getWindowBounds() );
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

