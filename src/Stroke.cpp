#include "cinder/Cinder.h"

#include "Resources.h"
#include "Stroke.h"

using namespace std;
using namespace cinder;

ci::gl::GlslProg Stroke::sShader;

Stroke::Stroke()
{
	if ( !sShader )
	{
		try
		{
			sShader = gl::GlslProg( app::loadResource( RES_STROKE_VERT ),
									app::loadResource( RES_STROKE_FRAG ),
									app::loadResource( RES_STROKE_GEOM ),
									GL_LINES_ADJACENCY_EXT, GL_TRIANGLE_STRIP, 7 );
		}
		catch( const std::exception &e )
		{
			app::console() << e.what() << std::endl;
		}
	}

    mThickness = 50.0f;
    mLimit = 0.75f;
	mColor = ColorA::white();
}

void Stroke::update( Vec2f point )
{
	if ( mPoints.empty() ||
		 ( point.distanceSquared( mPoints.back() ) > 2.f ) )
		mPoints.push_back( point );
}

void Stroke::draw()
{
	// create a new vector that can contain 3D vertices
	vector< Vec3f > vertices;

	// to improve performance, make room for the vertices + 2 adjacency vertices
	vertices.reserve( mPoints.size() + 2);

	// first, add an adjacency vertex at the beginning
	vertices.push_back( 2.0f * Vec3f( mPoints[ 0 ] ) - Vec3f( mPoints[ 1 ] ) );

	// next, add all 2D points as 3D vertices
	vector< Vec2f >::iterator itr;
	for ( itr = mPoints.begin(); itr != mPoints.end(); ++itr )
		vertices.push_back( Vec3f( *itr ) );

	// next, add an adjacency vertex at the end
	size_t n = mPoints.size();
	vertices.push_back( 2.0f * Vec3f( mPoints[ n - 1 ] ) - Vec3f( mPoints[ n - 2 ] ) );

	// now that we have a list of vertices, create the index buffer
	n = vertices.size() - 2;
	vector< uint32_t > indices;

	indices.reserve( n * 4 );
	for ( size_t i = 1 ; i < vertices.size() - 2; ++i )
	{
		indices.push_back( i - 1 );
		indices.push_back( i );
		indices.push_back( i + 1 );
		indices.push_back( i + 2 );
	}

	// finally, create the mesh
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();

	mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_LINES_ADJACENCY_EXT );
	mVboMesh.bufferPositions( &(vertices.front()), vertices.size() );
	mVboMesh.bufferIndices( indices );

	// bind the shader and send the mesh to the GPU
	if ( sShader && mVboMesh )
	{
		sShader.bind();
		sShader.uniform( "WIN_SCALE", mWindowSize ); // casting to Vec2f is mandatory!
		sShader.uniform( "MITER_LIMIT", mLimit );
		sShader.uniform( "THICKNESS", mThickness );

		gl::color( mColor );

		gl::draw( mVboMesh );

		sShader.unbind();
	}

}

