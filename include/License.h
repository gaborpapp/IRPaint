#pragma once

#include <string>
#include <map>
#include <vector>

namespace mndl { namespace license {

class License
{
	typedef std::map<std::string,std::string> Values;

	static const std::string PRODUCT;
	static const std::string DATA;
	static const std::string TIME;
	static const std::string MAC;
	static const std::string RANDOM;

	static const std::string TIME_LIMIT;
	static const std::string RESULT;

	static const std::string SEPARATOR;

public:
	void                init( ci::fs::path &xmlData );

	void                setKey( const ci::fs::path &publicKey );
	const ci::fs::path &getKey() const;

	void                setProduct( const std::string &product );
	const std::string  &getProduct() const;

	void                addServer( const std::string &server );
	int                 getServerCount() const;
	const std::string   getServer( int pos ) const;

	bool                process();

	const Values       &getResult() const;

protected:
	std::string genString( int length = 0 );
	void        addValue( Values &data, const std::string &key, const std::string &value );
	void        string2Values( const std::string &src, Values      &dst );
	void        values2String( const Values      &src, std::string &dst );
	std::string getTime();
	std::string getDate();
	std::string getMac();

protected:
	ci::fs::path             mPublicKey;
	std::string              mProduct;
	std::vector<std::string> mServers;
	Values                   mResult;
};

} } // namespace mndl::license
