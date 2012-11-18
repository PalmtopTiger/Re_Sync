#pragma once

#include <string>


namespace io
{
	void ReadFile(const std::string& filename, std::wstring& buffer);
	void WriteFile(const std::string& filename, const std::wstring& buffer, bool write_bom = true);
}
