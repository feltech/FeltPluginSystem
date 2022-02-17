#include "client.hpp"

namespace feltplugindemoplugin::client
{
String::String(std::string const & local) : String{local.c_str()} {}

String::String(char const * local)
{
	Base::create(local);
}

char const * String::c_str() const
{
	return suite_.c_str(handle_);
}

char String::at(int n) const
{
	return call(suite_.at, n);
}

String::operator std::string() const
{
	return std::string{c_str()};
}

String StringDict::at(String const & key)
{
	return String{call(suite_.at, static_cast<fpdemo_String_h>(key))};
}

void StringDict::insert(String const & key, String const & value)
{
	call(suite_.insert, static_cast<fpdemo_String_h>(key), static_cast<fpdemo_String_h>(value));
}
}  // namespace feltplugindemoplugin
