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

#include "cinder/Vector.h"

#include "Blob.h"
#include "PParams.h"

namespace mndl {

class BlobTracker;

class ManualCalibration
{
	public:
		ManualCalibration( BlobTracker *bt );

		void startCalibration();
		void stopCalibration();

		void update();
		void draw();

	private:
		BlobTracker *mBlobTrackerRef;

		void blobsBegan( BlobEvent event );

		//! Switches calibration of tracking with projection on and off
		void toggleCalibrationCB();

		bool mIsCalibrating;
		size_t mCalibrationGridIndex; //< current index in \a mCalibrationGrid
		ci::Vec2i mCalibrationGridSize;
		std::vector< ci::Vec2f > mCalibrationGrid; //< normalized coordinates of calibration points
		std::vector< ci::Vec2f > mCameraCalibrationGrid; //< normalized coordinates in camera image

		int mLastCalibrationIndexReceived; //< last point index calibrated in \a mCalibrationGrid
		ci::Vec2f mCalibrationPos; //< normalized blob position
		int32_t mCalibrationId; //< id of the calibration blob

		// params
		ci::params::PInterfaceGl mParams;
};

} // namespace mndl

