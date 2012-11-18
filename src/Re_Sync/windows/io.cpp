/*******************************************************************************
 * This file is part of Re_Sync.
 *
 * Copyright (C) 2011  Andrey Efremov <duxus@yandex.ru>
 *
 * Re_Sync is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Re_Sync is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Re_Sync.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#if defined _MSC_VER && _MSC_VER >= 1400
# ifndef _SCL_SECURE_NO_WARNINGS
#  define _SCL_SECURE_NO_WARNINGS
# endif
# ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
# endif
#endif

#include <cstring>
#include <locale>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <codecvt>

#include "../nullptr.h"
#include "../exception.h"
#include "io.h"


namespace io
{
	const size_t MAX_BOM = 3;

	/**********************************************/
	/*   Класс для определения кодовой страницы   */
	/**********************************************/
	class Codepage
	{
		size_t _bom_size;
		wchar_t _bom[MAX_BOM + 1u];
		std::codecvt<wchar_t, char, mbstate_t>* _facet;

	public:
		Codepage(const wchar_t bom[], std::codecvt<wchar_t, char, mbstate_t>* facet)
		{
			if (bom != nullptr)
			{
				_bom_size = wcsnlen(bom, MAX_BOM);
				if (_bom_size != 2u && _bom_size != 3u)
				{
					BOOST_THROW_EXCEPTION(
						boost::enable_error_info(std::runtime_error("Wrong BOM"))
						<< error_message(L"Неправильный BOM")
					);
				}
				wcsncpy(_bom, bom, _bom_size);
			}
			else
			{
				_bom_size = 0u;
			}

			_facet = facet;
		}
		
		bool isMyBOM(const wchar_t bom[])
		{
			return _bom_size == 0u || wcsncmp(bom, _bom, _bom_size) == 0;
		}

		std::codecvt<wchar_t, char, mbstate_t>* getFacet() { return _facet; }
		size_t getBOMSize() { return _bom_size; }
	};

	/********************/
	/*   Чтение файла   */
	/********************/
	void ReadFile(const std::string& filename, std::wstring& buffer)
	{
		wchar_t buf[MAX_BOM + 1u];
		std::wifstream wfin(filename.c_str(), std::ios_base::binary);
		wfin.imbue(std::locale::classic());
		if (!wfin.is_open())
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Сan't open file for reading"))
				<< error_message(L"Ошибка открытия файла для чтения")
			);
		}

		wfin.read(buf, MAX_BOM);
		if (!wfin.good())
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("File too small"))
				<< error_message(L"Слишком маленький файл")
			);
		}
		wfin.seekg(0);

		std::codecvt<wchar_t, char, mbstate_t>* facet = nullptr;

#if _MSC_VER >= 1600
		// MSVC >= 2010
		Codepage utf8(L"\xEF\xBB\xBF", new std::codecvt_utf8<wchar_t, 0x10ffffUL, std::consume_header>);
		if (facet == nullptr && utf8.isMyBOM(buf))
		{
			facet = utf8.getFacet();
		}

		Codepage utf16le(L"\xFF\xFE", new std::codecvt_utf16<wchar_t, 0x10ffffUL, std::codecvt_mode(std::little_endian | std::consume_header)>);
		if (facet == nullptr && utf16le.isMyBOM(buf))
		{
			facet = utf16le.getFacet();
		}
		
		Codepage utf16be(L"\xFE\xFF", new std::codecvt_utf16<wchar_t, 0x10ffffUL, std::consume_header>);
		if (facet == nullptr && utf16be.isMyBOM(buf))
		{
			facet = utf16be.getFacet();
		}
#endif

#if _MSC_VER >= 1310
		// MSVC >= 2003
		if (facet == nullptr) facet = new std::codecvt_byname<wchar_t, char, mbstate_t>(std::locale(".ACP").name());
#endif

		if (facet == nullptr)
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("File encoding not detected"))
				<< error_message(L"Неизвестная кодировка файла")
			);
		}

		wfin.imbue(std::locale(std::locale::classic(), facet));

		std::wostringstream woss;
		woss << wfin.rdbuf();
		wfin.close();

		woss.str().swap(buffer);
	}

	/********************/
	/*   Запись файла   */
	/********************/
	void WriteFile(const std::string& filename, const std::wstring& buffer, bool write_bom)
	{
		std::wofstream wfout(filename.c_str(), std::ios_base::binary);
		if (!wfout.is_open())
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Сan't open file for writing"))
				<< error_message(L"Ошибка открытия файла для записи")
			);
		}

		std::codecvt<wchar_t, char, mbstate_t>* facet = nullptr;

#if _MSC_VER >= 1600
		// MSVC >= 2010
		if (write_bom)
		{
			facet = new std::codecvt_utf8<wchar_t, 0x10ffffUL, std::generate_header>;
		}
		else
		{
			facet = new std::codecvt_utf8<wchar_t>;
		}
#elif _MSC_VER >= 1310
		// MSVC >= 2003
		facet = new std::codecvt_byname<wchar_t, char, mbstate_t>(std::locale(".ACP").name());
#endif

		if (facet == nullptr)
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Can't convert encoding"))
				<< error_message(L"Ошибка преобразования кодировки")
			);
		}
		
		wfout.imbue(std::locale(std::locale::classic(), facet));

		wfout << buffer;
		wfout.close();
	}
}
