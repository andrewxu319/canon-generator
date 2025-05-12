#pragma once

#include <string>
#include <string_view>

class Exception {
public:
	Exception(std::string_view error)
		: m_error{ error } {
	}

	const std::string_view getError() const {
		return m_error;
	}

private:
	std::string m_error;
};