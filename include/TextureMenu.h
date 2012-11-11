#pragma once

#include <vector>

#include "cinder/Cinder.h"

#include <boost/bind.hpp>
#include <boost/signals2/signal.hpp>

#include "cinder/gl/Texture.h"
#include "cinder/Area.h"
#include "cinder/Function.h"
#include "cinder/ImageIo.h"
#include "cinder/Vector.h"

namespace mndl { namespace gl {

class TextureMenu
{
	public:
		TextureMenu() {}
		TextureMenu( ci::ImageSourceRef background );

		void setPosition( const ci::Vec2i &pos );
		ci::Vec2i getPosition() const;

		ci::Vec2i getSize() const;
		ci::Area getBounds() const;

		void show( bool visible = true );
		void hide();
		bool isVisible() const;

		void processClick( const ci::Vec2i &pos );

		void draw() const;

	protected:
		typedef boost::signals2::signal< void() > ButtonSignal;

		struct Button {
			Button( ci::ImageSourceRef buttonOn, ci::ImageSourceRef buttonOff );

			ci::gl::Texture mTextureOn;
			ci::gl::Texture mTextureOff;
			ci::Area mArea;
			bool mIsOn;

			ButtonSignal mSignal;
		};
		typedef std::shared_ptr< Button > ButtonRef;

		struct Obj
		{
			Obj() : mPosition( ci::Vec2i::zero() ), mVisible( true ) {}

			ci::gl::Texture mBackground;
			ci::Vec2i mPosition;
			std::vector< ButtonRef > mButtons;
			bool mVisible;
		};

		std::shared_ptr< Obj > mObj;

	public:
		template< typename T >
		void addButton( ci::ImageSourceRef buttonOn, ci::ImageSourceRef buttonOff,
				void( T::*fn )(), T *obj )
		{
			ButtonRef bref = ButtonRef( new Button( buttonOn, buttonOff ) );
			bref->mSignal.connect( std::function< void() >( boost::bind( fn, obj ) ) );
			mObj->mButtons.push_back( bref );
		}
};

} } // namespace mndl::gl
