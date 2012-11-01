#include <ctime>
#include <sstream>
#include <iomanip>

#include "cinder/app/App.h"
#include "cinder/ImageIo.h"

#include "Utils.h"

using namespace ci;
using namespace std;

namespace mndl
{

std::string getTimestamp()
{
	struct tm tm;
	time_t ltime;
	static int last_sec = 0;
	static int index = 0;

	time( &ltime );
#if defined( CINDER_MAC )
	localtime_r( &ltime, &tm );
#elif defined( CINDER_MSW )
	localtime_s( &tm, &ltime );
#endif
	if ( last_sec != tm.tm_sec )
		index = 0;

	stringstream ss;
	ss << setfill('0') << setw(2) << tm.tm_year - 100 <<
		setw(2) << tm.tm_mon + 1 << setw(2) << tm.tm_mday <<
		setw(2) << tm.tm_hour << setw(2) << tm.tm_min <<
		setw(2) << tm.tm_sec << setw(2) << index;

	index++;
	last_sec = tm.tm_sec;

	return ss.str();
}

vector< gl::Texture > loadTextures( const fs::path &relativeDir )
{
	vector< gl::Texture > textures;

	fs::path dataPath = app::getAssetPath( relativeDir );

	for (fs::directory_iterator it( dataPath ); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".png"))
		{
			gl::Texture t = loadImage( app::loadAsset( relativeDir / it->path().filename() ) );
			textures.push_back( t );
		}
	}

	return textures;
}

string getMinMaxStepString( int min, int max, int step )
{
	stringstream st;
	st << "min=" << min << " max=" << max << " step=" << step;
	return st.str();
}

} // namespace cinder

