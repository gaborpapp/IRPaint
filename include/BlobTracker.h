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

#pragma once

#include <vector>

#include <boost/signals2/signal.hpp>

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/qtime/MovieWriter.h"
#include "cinder/qtime/QuickTime.h"

#include "cinder/Capture.h"
#include "cinder/Function.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

#include "Blob.h"
#include "ManualCalibration.h"
#include "PParams.h"

namespace mndl {

class BlobTracker
{
	public:
		BlobTracker() :
			mSavingVideo( false ),
			mIdCounter( 1 )
		{}

		void setup();
		void update();
		void draw();
		void shutdown();

		typedef void( BlobCallback )( BlobEvent );
		typedef boost::signals2::signal< BlobCallback > BlobSignal;

		template< typename T >
		boost::signals2::connection registerBlobsBegan( void( T::*fn )( BlobEvent ), T *obj )
		{
			return mBlobsBeganSig.connect( std::function< BlobCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		boost::signals2::connection registerBlobsMoved( void( T::*fn )( BlobEvent ), T *obj )
		{
			return mBlobsMovedSig.connect( std::function< BlobCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		boost::signals2::connection registerBlobsEnded( void( T::*fn )( BlobEvent ), T *obj )
		{
			return mBlobsEndedSig.connect( std::function< BlobCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		void registerBlobsCallbacks( void( T::*fnBegan )( BlobEvent ),
				void( T::*fnMoved )( BlobEvent ),
				void( T::*fnEnded )( BlobEvent ),
				T *obj )
		{
			registerBlobsBegan< T >( fnBegan, obj );
			registerBlobsMoved< T >( fnMoved, obj );
			registerBlobsEnded< T >( fnEnded, obj );
		}

		void unregisterBlobsCallback( boost::signals2::connection connection )
		{
			connection.disconnect();
		}

		size_t getBlobNum() const;
		ci::Rectf getBlobBoundingRect( size_t i ) const;
		ci::Vec2f getBlobCentroid( size_t i ) const;

		std::shared_ptr< ManualCalibration > getCalibrator() const
		{
			return mCalibratorRef;
		}

	private:
		enum {
			SOURCE_RECORDING = 0,
			SOURCE_CAMERA
		};

		void setupGui();
		void playVideoCB();
		void saveVideoCB();

		// capture
		ci::Capture mCapture;
		ci::gl::Texture mTextureOrig;
		ci::gl::Texture mTextureBlurred;
		ci::gl::Texture mTextureThresholded;

		std::vector< ci::Capture > mCaptures;
		std::vector< std::string > mDeviceNames;

        ci::qtime::MovieSurface mMovie;
		ci::qtime::MovieWriter mMovieWriter;
		bool mSavingVideo;

		int mSource; // recording or camera

		static const int CAPTURE_WIDTH = 640;
		static const int CAPTURE_HEIGHT = 480;

		int mCurrentCapture;

		enum {
			DRAW_NONE = 0,
			DRAW_ORIGINAL,
			DRAW_BLURRED,
			DRAW_THRESHOLDED
		};
		int mDrawCapture;

		bool mFlip;
		int mThreshold;
		int mBlurSize;
		float mMinArea;
		float mMaxArea;

		std::vector< BlobRef > mBlobs;
		void trackBlobs( std::vector< BlobRef > newBlobs );
		int32_t findClosestBlobKnn( const std::vector< BlobRef > &newBlobs,
				BlobRef track, int k, double thresh );
		int32_t mIdCounter;

		// signals
		BlobSignal mBlobsBeganSig;
		BlobSignal mBlobsMovedSig;
		BlobSignal mBlobsEndedSig;

		std::shared_ptr< ManualCalibration > mCalibratorRef;

		// normalizes blob coordinates from camera 2d coords to [0, 1]
		ci::RectMapping mNormMapping;

		// params
		ci::params::PInterfaceGl mParams;
};

} // namespace mndl

