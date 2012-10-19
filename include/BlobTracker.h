#pragma once

#include <vector>

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
			ci::Vec2f mLastCentroid;
		};
		typedef std::shared_ptr< Blob > BlobRef;

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

		// normalizes blob coordinates from camera 2d coords to [0, 1]
		ci::RectMapping mNormMapping;

		// params
		ci::params::PInterfaceGl mParams;
};

} // namespace mndl

