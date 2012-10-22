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
#include "cinder/Rect.h"
#include "cinder/Vector.h"

#include "PParams.h"

namespace mndl {

class BlobTracker
{
	public:
		BlobTracker() :
			mSavingVideo( false ),
			mIdCounter( 1 ) {}

		void setup();
		void update();
		void draw();
		void shutdown();

		struct Blob
		{
			Blob() : mId( -1 ) {}

			int32_t mId;
			ci::Rectf mBbox;
			ci::Vec2f mCentroid;
			ci::Vec2f mPrevCentroid;
		};
		typedef std::shared_ptr< Blob > BlobRef;

		//! Represents a blob event
		class BlobEvent
		{
			public:
				BlobEvent( BlobRef blobRef ) : mBlobRef( blobRef ) {}

				//! Returns an ID unique for the lifetime of the blob
				int32_t getId() const { return mBlobRef->mId; }
				//! Returns the x position of the blob centroid normalized to the image source width
				float getX() const { return mBlobRef->mCentroid.x; }
				//! Returns the y position of the blob centroid normalized to the image source height
				float getY() const { return mBlobRef->mCentroid.y; }
				//! Returns the position of the blob centroid normalized to the image resolution
				ci::Vec2f getPos() const { return mBlobRef->mCentroid; }
				//! Returns the previous x position of the blob centroid normalized to the image source width
				float getPrevX() const { return mBlobRef->mPrevCentroid.x; }
				//! Returns the previous y position of the blob centroid normalized to the image source height
				float getPrevY() const { return mBlobRef->mPrevCentroid.y; }
				//! Returns the previous position of the blob centroid normalized to the image resolution
				ci::Vec2f getPrevPos() const { return mBlobRef->mPrevCentroid; }

			private:
				BlobRef mBlobRef;
		};

		typedef void( BlobCallback )( BlobEvent );
		typedef boost::signals2::signal< BlobCallback > BlobSignal;

		template< typename T >
		void registerBlobsBegan( void( T::*fn )( BlobEvent ), T *obj )
		{
			mBlobsBeganSig.connect( std::function< BlobCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		void registerBlobsMoved( void( T::*fn )( BlobEvent ), T *obj )
		{
			mBlobsMovedSig.connect( std::function< BlobCallback >( std::bind( fn, obj, std::_1 ) ) );
		}
		template< typename T >
		void registerBlobsEnded( void( T::*fn )( BlobEvent ), T *obj )
		{
			mBlobsEndedSig.connect( std::function< BlobCallback >( std::bind( fn, obj, std::_1 ) ) );
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

		size_t getBlobNum() const;
		ci::Rectf getBlobBoundingRect( size_t i ) const;
		ci::Vec2f getBlobCentroid( size_t i ) const;

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

		// normalizes blob coordinates from camera 2d coords to [0, 1]
		ci::RectMapping mNormMapping;

		// params
		ci::params::PInterfaceGl mParams;
};

} // namespace mndl

