#pragma once

#include <vector>

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/Capture.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

#include "PParams.h"

namespace mndl {

class BlobTracker
{
	public:
		void setup();
		void update();
		void draw();
		void shutdown();

		struct Blob
		{
			ci::Rectf bbox;
			ci::Vec2f centroid;
		};

		size_t getBlobNum() const;
		ci::Rectf getBlobBoundingRect( size_t i ) const;
		ci::Vec2f getBlobCentroid( size_t i ) const;

	private:
		// capture
		ci::Capture mCapture;
		ci::gl::Texture mTextureOrig;
		ci::gl::Texture mTextureBlurred;
		ci::gl::Texture mTextureThresholded;

		std::vector< ci::Capture > mCaptures;

		static const int CAPTURE_WIDTH = 640;
		static const int CAPTURE_HEIGHT = 480;

		int mCurrentCapture;

		bool mDrawCapture;

		bool mFlip;
		int mThreshold;
		int mBlurSize;
		float mMinArea;
		float mMaxArea;

		std::vector< Blob > mBlobs;

		// normalizes blob coordinates from camera 2d coords to [0, 1]
		ci::RectMapping mNormMapping;

		// params
		ci::params::PInterfaceGl mParams;
};

} // namespace mndl

