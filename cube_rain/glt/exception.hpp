#pragma once
#include <string>
#include <stdexcept>

namespace glt::shader {

struct exception : public std::runtime_error
{
	exception(std::string const & msg) : std::runtime_error(msg) {}
};

}  // glt::shader
