#include "cinder/gl/gl.h"

#include "TextureMenu.h"

using namespace ci;
using namespace std;

namespace mndl { namespace gl {

TextureMenu::TextureMenu( ImageSourceRef background ) :
	mObj( shared_ptr< Obj >( new Obj ) )
{
	mObj->mBackground = ci::gl::Texture( background );
}

void TextureMenu::setPosition( const ci::Vec2i &pos )
{
	mObj->mPosition = pos;
}

ci::Vec2i TextureMenu::getPosition() const
{
	return mObj->mPosition;
}

Vec2i TextureMenu::getSize() const
{
	return mObj->mBackground.getSize();
}

Area TextureMenu::getBounds() const
{
	return mObj->mBackground.getBounds() + mObj->mPosition;
}

void TextureMenu::show( bool visible )
{
	mObj->mVisible = visible;
}

void TextureMenu::hide()
{
	mObj->mVisible = false;
}

bool TextureMenu::isVisible() const
{
	return mObj->mVisible;
}

void TextureMenu::draw() const
{
	if ( !isVisible() )
		return;

	Area bounds = getBounds();
	ci::gl::enableAlphaBlending();
	ci::gl::color( Color::white() );
	ci::gl::draw( mObj->mBackground, bounds );
	for ( vector< ButtonRef >::const_iterator it = mObj->mButtons.begin();
			it != mObj->mButtons.end(); ++it )
	{
		if ( (*it)->mIsOn )
			ci::gl::draw( (*it)->mTextureOn, bounds );
		else
			ci::gl::draw( (*it)->mTextureOff, bounds );
	}
	ci::gl::disableAlphaBlending();
}

void TextureMenu::processClick( const ci::Vec2i &pos )
{
	if ( !isVisible() )
		return;

	Vec2i p = pos - getPosition();

	for ( vector< ButtonRef >::const_iterator it = mObj->mButtons.begin();
			it != mObj->mButtons.end(); ++it )
	{
		(*it)->mIsOn = (*it)->mArea.contains( p );

		if ( (*it)->mIsOn )
			(*it)->mSignal();
	}
}

TextureMenu::Button::Button( ImageSourceRef buttonOn, ImageSourceRef buttonOff ) :
	mIsOn( false )
{
	mTextureOn = ci::gl::Texture( buttonOn );
	mTextureOff = ci::gl::Texture( buttonOff );

	Surface surf( buttonOn );
	Surface::ConstIter it = surf.getIter();
	Rectf bounds;
	bool rectInited = false;
	while ( it.line() )
	{
		while ( it.pixel() )
		{
			if ( it.a() )
			{
				Vec2i p = it.getPos();
				if ( rectInited )
				{
					bounds.include( p );
				}
				else
				{
					bounds = Rectf( p, p );
					rectInited = true;
				}
			}
		}
	}
	mArea = Area( bounds );
}

} } // namespace mndl::gl
