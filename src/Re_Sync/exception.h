#pragma once

#include <stdexcept>
#include <boost/exception/all.hpp>

typedef boost::error_info<struct tag_error_message, std::wstring> error_message;
