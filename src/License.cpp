#include "cinder/app/AppBasic.h"
#include "cinder/Xml.h"
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "Curl.h"
#include "Crypter.h"
#include "license.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl::license;
using namespace mndl::crypter;
using namespace mndl::curl;

const string License::PRODUCT   = "PRODUCT";
const string License::DATA      = "DATA";
const string License::TIME      = "TIME";
const string License::MAC       = "MAC";
const string License::RANDOM    = "RANDOM";

const string License::SEPARATOR = ":::";

const string License::TIME_LIMIT = "TIME_LIMIT";
const string License::RESULT     = "RESULT";

void License::init( ci::fs::path &xmlData )
{
	XmlTree doc( loadFile( xmlData ));

	init( doc );
}

void License::init( XmlTree &doc )
{
	if( doc.hasChild( "License" ))
	{
		XmlTree xmlLicense = doc.getChild( "License" );

		string product = xmlLicense.getAttributeValue<string>( "Product", ""  );
		string key     = xmlLicense.getAttributeValue<string>( "Key"    , ""  );

		setProduct( product );
		setKeyPath( getAssetPath( key ));

		for( XmlTree::Iter child = xmlLicense.begin(); child != xmlLicense.end(); ++child )
		{
			if( child->getTag() == "Server" )
			{
				string server = child->getAttributeValue<string>( "Name" );
				addServer( server );
			}
		}
	}
}

void License::setKeyPath( const fs::path &publicKeyPath )
{
	mPublicKeyPath = publicKeyPath;
}

const fs::path &License::getKeyPath() const
{
	return mPublicKeyPath;
}

void License::setKey( const std::string &publicKey )
{
	mPublicKey = publicKey;
}

const std::string &License::getKey() const
{
	return mPublicKey;
}

void License::setProduct( const string &product )
{
	mProduct = product;
}

const string &License::getProduct() const
{
	return mProduct;
}

void License::addServer( const string &server )
{
	mServers.push_back( server );
}

int License::getServerCount() const
{
	return mServers.size();
}

const string License::getServer( int pos ) const
{
	if( pos < 0
	 && pos >= getServerCount())
		return "";

	return mServers[pos];
}

bool License::process()
{
	if( getServerCount() == 0
	 || ( getKeyPath().empty() && getKey().empty())
	 || getProduct().empty())
		return false;

	string ret;
	Values value;
	vector<string> valueSend;
	string data;
	string dataEncrypt;
	string dataDecrypt;
	string dataRandom = genString();

	addValue( value, RANDOM, dataRandom );
	addValue( value, TIME  , getDate() + getTime());
//	addValue( value, MAC   , getMac());

	values2String( value, data );
	if( ! getKey().empty())
		dataEncrypt = Crypter::rsaPublicEncryptDirect( getKey(), data );
	else
		dataEncrypt = Crypter::rsaPublicEncrypt( getKeyPath(), data );

	valueSend.push_back( getProduct());
	valueSend.push_back( dataEncrypt );

	for( vector<string>::iterator it = mServers.begin(); it != mServers.end(); ++it )
	{
		ret = curl::Curl::post( *it, valueSend );

		if( ! ret.empty())
		{
			if( ! getKey().empty())
				dataDecrypt = Crypter::rsaPublicDencryptDirect( getKey(), ret );
			else
				dataDecrypt = Crypter::rsaPublicDencrypt( getKeyPath(), ret );

			if( ! dataDecrypt.empty())
				break;
		}
	}

	string2Values( dataDecrypt, mResult );

	if( mResult[ RANDOM ] == dataRandom
	 && mResult[ RESULT ] == "PASS" )
		return true;

	return false;
}

const License::Values &License::getResult() const
{
	return mResult;
}

string License::genString( int length /* = 0 */ )
{
	string ret = "";

	if( length <= 0 )
		length = 20;

	string time = getTime();
	srand( atoi( time.c_str()));

	for( int pos = 0; pos < length; ++pos )
	{
		switch( rand() % 3 )
		{
		case 0: ret += ( '0' + rand() % 10 ); break; // number
		case 1: ret += ( 'a' + rand() % 26 ); break; // letter
		case 2: ret += ( 'A' + rand() % 26 ); break; // capital letter
		}
	}

	return ret;
}

void License::addValue( Values &data, const string &key, const string &value )
{
	data.insert( make_pair<string,string>( key, value ));
}

void License::string2Values( const string &src, Values &dst )
{
	if( src.empty())
		return;

	boost::char_separator< char > sep( SEPARATOR.c_str());
	boost::tokenizer< boost::char_separator< char > > tokens( src, sep );
	BOOST_FOREACH( string t, tokens )
	{
		size_t posEqual = t.find( '=' );

		if( posEqual != string::npos )
		{
			string key   = t.substr( 0, posEqual  );
			string value = t.substr( posEqual + 1 );

			addValue( dst, key, value );
		}
	}
}

void License::values2String( const Values &src, string &dst )
{
	dst = "";

	for( Values::const_iterator it = src.begin(); it != src.end(); ++it )
	{
		if( it != src.begin())
			dst += SEPARATOR;

		dst += it->first + "=" + it->second;
	}
}

string License::getTime()
{
	int time = 
	boost::posix_time::second_clock::local_time().time_of_day().hours() * 10000 +
	boost::posix_time::second_clock::local_time().time_of_day().minutes() * 100 +
	boost::posix_time::second_clock::local_time().time_of_day().seconds();

	stringstream out;
	out << time;
	return out.str();
}

string License::getDate()
{
	int date = 
	boost::posix_time::second_clock::local_time().date().year() * 10000 +
	boost::posix_time::second_clock::local_time().date().month() * 100 +
	boost::posix_time::second_clock::local_time().date().day();

	stringstream out;
	out << date;
	return out.str();
}

string License::getMac()
{
	return "";
}
