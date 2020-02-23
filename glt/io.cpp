#include <fstream>
#include <sstream>
#include <exception>
#include <boost/format.hpp>
#include "io.hpp"

using std::string,
	std::ifstream,
	std::stringstream,
	std::runtime_error;

using boost::format,
	boost::str;

string read_file(string const & fname)
{
	ifstream in(fname);
	if (!in.is_open())
		throw runtime_error(str(format("can't open '%1%' file") % fname));

	stringstream ss;
	ss << in.rdbuf();
	in.close();
	return ss.str();
}
