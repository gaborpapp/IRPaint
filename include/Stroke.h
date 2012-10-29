#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Color.h"
#include "cinder/Vector.h"

class Stroke
{
	 public:
		 Stroke();

		 void resize( ci::app::ResizeEvent event ) { mWindowSize = ci::Vec2f( event.getSize() ); }

		 void update( ci::Vec2f point );
		 void draw();

		 void clear() { mPoints.clear(); }

		 void setColor( ci::ColorA c ) { mColor = c; }
		 void setThickness( float t ) { mThickness = t; }

	private:
		float mThickness;
		float mLimit;
		ci::ColorA mColor;

		std::vector< ci::Vec2f > mPoints;
		static ci::gl::GlslProg sShader;

		ci::Vec2f mWindowSize;

		ci::gl::VboMesh mVboMesh;
};

