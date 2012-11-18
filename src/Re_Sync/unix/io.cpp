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

#include <cstring>
#include <locale>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>

#include "../nullptr.h"
#include "../exception.h"
#include "io.h"
#include "../codecvt/codecvt_cp1251.hpp"


namespace io
{
	const size_t MAX_BOM = 3;

	/**********************************************/
	/*   Класс для определения кодовой страницы   */
	/**********************************************/
	class Codepage
	{
		size_t _bom_size;
		char _bom[MAX_BOM + 1u];
		std::codecvt<wchar_t, char, mbstate_t>* _facet;

	public:
		Codepage(const char bom[], std::codecvt<wchar_t, char, mbstate_t>* facet)
		{
			if (bom != nullptr)
			{
				_bom_size = strnlen(bom, MAX_BOM);
				if (_bom_size != 2u && _bom_size != 3u)
				{
					BOOST_THROW_EXCEPTION(
						boost::enable_error_info(std::runtime_error("Wrong BOM"))
						<< error_message(L"Неправильный BOM")
					);
				}
				strncpy(_bom, bom, _bom_size);
			}
			else
			{
				_bom_size = 0u;
			}

			_facet = facet;
		}

		bool isMyBOM(const char bom[])
		{
			return _bom_size == 0u || strncmp(bom, _bom, _bom_size) == 0;
		}

		std::codecvt<wchar_t, char, mbstate_t>* getFacet() { return _facet; }
		size_t getBOMSize() { return _bom_size; }
	};

	/********************/
	/*   Чтение файла   */
	/********************/
	void ReadFile(const std::string& filename, std::wstring& buffer)
	{
		char buf[MAX_BOM + 1u];
		std::ifstream fin(filename.c_str(), std::ios_base::binary);
		fin.imbue(std::locale::classic());
		if (!fin.is_open())
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Сan't open file for reading"))
				<< error_message(L"Ошибка открытия файла для чтения")
			);
		}

		fin.read(buf, MAX_BOM);
		if (!fin.good())
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("File too small"))
				<< error_message(L"Слишком маленький файл")
			);
		}
		fin.close();

		std::wifstream wfin(filename.c_str(), std::ios_base::binary);
		if (!wfin.is_open())
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Сan't reopen file for reading"))
				<< error_message(L"Ошибка переоткрытия файла для чтения")
			);
		}

		std::codecvt<wchar_t, char, mbstate_t>* facet = nullptr;

		Codepage utf8("\xEF\xBB\xBF", new std::codecvt_byname<wchar_t, char, mbstate_t>("ru_RU.UTF-8"));
		if (facet == nullptr && utf8.isMyBOM(buf))
		{
			facet = utf8.getFacet();
			wfin.seekg(utf8.getBOMSize());
		}
		
		Codepage utf16le("\xFF\xFE", nullptr);
		if (facet == nullptr && utf16le.isMyBOM(buf))
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("UTF-16LE not supported yet"))
				<< error_message(L"UTF-16LE пока не поддерживается")
			);
		}
		
		Codepage utf16be("\xFE\xFF", nullptr);
		if (facet == nullptr && utf16be.isMyBOM(buf))
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("UTF-16BE not supported yet"))
				<< error_message(L"UTF-16BE пока не поддерживается")
			);
		}

		if (facet == nullptr) facet = new codecvt_cp1251;
		
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
		std::wofstream wfout;
		if (write_bom)
		{
			std::ofstream fout(filename.c_str(), std::ios_base::binary);
			fout.imbue(std::locale::classic());
			if (!fout.is_open())
			{
				BOOST_THROW_EXCEPTION(
					boost::enable_error_info(std::runtime_error("Сan't open file for writing"))
					<< error_message(L"Ошибка открытия файла для записи")
				);
			}
			fout.write("\xEF\xBB\xBF", 3);
			fout.close();

			wfout.open(filename.c_str(), std::ios_base::binary | std::ios_base::app);
		}
		else
		{
			wfout.open(filename.c_str(), std::ios_base::binary);
		}

		if (!wfout.is_open())
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Сan't reopen file for writing"))
				<< error_message(L"Ошибка переоткрытия файла для записи")
			);
		}

		std::codecvt<wchar_t, char, mbstate_t>* facet = nullptr;
		facet = new std::codecvt_byname<wchar_t, char, mbstate_t>("ru_RU.UTF-8");
		
		wfout.imbue(std::locale(std::locale::classic(), facet));

		wfout << buffer;
		wfout.close();
	}
}
