/*******************************************************************************
 * Re_Sync:
 * Synchronize subtitles with the video using the timing of other subtitles.
 *
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
#endif

#include <cstdlib>
#include <locale>
#include <iostream>

#include "nullptr.h"
#include "exception.h"
#include "format.h"
#include "structure.h"
#include "resync.h"

#ifdef __GNUC__
# include <getopt.h>
# include "unix/io.h"
#else
# include "glibc/getopt.h"
# include "windows/io.h"
#endif

#ifdef _MSC_VER
# define CONSOLE_LOCALE ".OCP"
#else
# define CONSOLE_LOCALE ""
#endif

// В милисекундах
int MIN_DURATION = 500;
int MAX_OFFSET = 5000;
int MAX_DESYNC = 200;
int MAX_SHIFT = 15000;
bool SKIP_LYRICS = false;
bool NO_SKIP = false;
bool ALLOW_OVERLAP = false;

void PrintHelp(char exec_name[]);


int main(int argc, char* argv[])
{
	// Кодировка для функций C++ (ios, boost, regex)
	std::locale::global( std::locale(CONSOLE_LOCALE) );

	// Обработка параметров
	bool verbose = false, generate_svg = false;
	std::string sync_name, desync_name, out_name, svg_name = "graph.svg";

#ifdef _DEBUG
	sync_name = "sync.ass"; desync_name = "desync.ass"; out_name = "output.ass";
	verbose = true; generate_svg = true;
#else
	{
		const char* short_options = "hvs:d:o:g::";

		enum {CODE_MIN_DURATION = 1000, CODE_MAX_OFFSET, CODE_MAX_DESYNC, CODE_MAX_SHIFT, CODE_SKIP_LYRICS, CODE_NO_SKIP, CODE_ALLOW_OVERLAP};
		const struct option long_options[] = {
			{"help",         no_argument,       nullptr, 'h'},
			{"verbose",      no_argument,       nullptr, 'v'},
			{"sync",         required_argument, nullptr, 's'},
			{"desync",       required_argument, nullptr, 'd'},
			{"output",       required_argument, nullptr, 'o'},
			{"graph",        optional_argument, nullptr, 'g'},
			{"min-duration", required_argument, nullptr, CODE_MIN_DURATION},
			{"max-offset",   required_argument, nullptr, CODE_MAX_OFFSET},
			{"max-desync",   required_argument, nullptr, CODE_MAX_DESYNC},
			{"max-shift",    required_argument, nullptr, CODE_MAX_SHIFT},
			{"skip-lyrics",  no_argument,       nullptr, CODE_SKIP_LYRICS},
			{"no-skip",      no_argument,       nullptr, CODE_NO_SKIP},
			{"allow-overlap",no_argument,       nullptr, CODE_ALLOW_OVERLAP},
			{nullptr, 0, nullptr, 0}
		};

		int ret, value;
		while ( (ret = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1 )
		{
			switch (ret)
			{
			case 'h':
				PrintHelp(argv[0]);
				return EXIT_SUCCESS;

			case 'v':
				verbose = true;
				break;

			case 's':
				sync_name = optarg;
				break;

			case 'd':
				desync_name = optarg;
				break;

			case 'o':
				out_name = optarg;
				break;

			case 'g':
				if (optarg != nullptr)
				{
					svg_name = optarg;
				}
				generate_svg = true;
				break;

			case CODE_MIN_DURATION:
				value = atoi(optarg);
				if (value >= 0)
				{
					MIN_DURATION = value;
				}
				else
				{
					std::wclog << L"MIN_DURATION не может быть отрицательным" << std::endl;
				}
				break;

			case CODE_MAX_OFFSET:
				value = atoi(optarg);
				if (value >= 0)
				{
					MAX_OFFSET = value;
				}
				else
				{
					std::wclog << L"MAX_OFFSET не может быть отрицательным" << std::endl;
				}
				break;

			case CODE_MAX_DESYNC:
				value = atoi(optarg);
				if (value >= 0)
				{
					MAX_DESYNC = value;
				}
				else
				{
					std::wclog << L"MAX_DESYNC не может быть отрицательным" << std::endl;
				}
				break;

			case CODE_MAX_SHIFT:
				value = atoi(optarg);
				if (value >= 0)
				{
					MAX_SHIFT = value;
				}
				else
				{
					std::wclog << L"MAX_SHIFT не может быть отрицательным" << std::endl;
				}
				break;

			case CODE_SKIP_LYRICS:
				SKIP_LYRICS = true;
				break;

			case CODE_NO_SKIP:
				NO_SKIP = true;
				break;

			case CODE_ALLOW_OVERLAP:
				ALLOW_OVERLAP = true;
				break;

			default:
				break;
			}
		}
	}
#endif

	if (sync_name.empty())
	{
		std::wcerr << L"Не указан синхронизированный скрипт\n" << std::endl;
		PrintHelp(argv[0]);
		return EXIT_FAILURE;
	}
	if (desync_name.empty())
	{
		std::wcerr << L"Не указан рассинхронизированный скрипт\n" << std::endl;
		PrintHelp(argv[0]);
		return EXIT_FAILURE;
	}
	if (out_name.empty())
	{
		std::wcerr << L"Не указан выходной скрипт\n" << std::endl;
		PrintHelp(argv[0]);
		return EXIT_FAILURE;
	}

	try
	{
		//
		// Синхронизированный
		//
		if (verbose) std::wclog << L"Чтение \"" << sync_name.c_str() << L"\"" << std::endl;
		std::wstring sync_content;
		io::ReadFile(sync_name, sync_content);

		if (verbose) std::wclog << L"Разбор скрипта" << std::endl;
		format::srt::Phrases sync_phrases;
		format::ass::Script sync_script;
		PhrasesPtrVector sync_pPhrases;
		format::Format sync_format = format::DetectFormat(sync_content);
		switch (sync_format)
		{
		case format::FMT_SRT:
			if (verbose) std::wclog << L"Формат: SRT" << std::endl;
			format::srt::Parse(sync_content, sync_phrases);
			for (format::srt::Phrases::size_type i = 0; i < sync_phrases.size(); ++i)
			{
				sync_pPhrases.push_back( &(sync_phrases[i]) );
			}
			break;

		case format::FMT_ASS:
			if (verbose) std::wclog << L"Формат: SSA/ASS" << std::endl;
			format::ass::Parse(sync_content, sync_script);
			for (format::ass::Events::size_type i = 0; i < sync_script.meta_events.events.size(); ++i)
			{
				sync_pPhrases.push_back( &(sync_script.meta_events.events[i]) );
			}
			break;

		default:
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Format not supported"))
				<< error_message(L"Формат не поддерживается")
			);
		}

		if (verbose) std::wclog << L"Группировка фраз" << std::endl;
		PhraseGroups sync_groups;
		GroupPhrases(sync_pPhrases, sync_groups);
		if (sync_groups.size() < 1)
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Phrase groups has not been formed"))
				<< error_message(L"Не сформировано ни одной группы")
			);
		}

		//
		// Рассинхронизированный
		//
		if (verbose) std::wclog << L"Чтение \"" << desync_name.c_str() << L"\"" << std::endl;
		std::wstring desync_content;
		io::ReadFile(desync_name, desync_content);

		if (verbose) std::wclog << L"Разбор скрипта" << std::endl;
		format::srt::Phrases desync_phrases;
		format::ass::Script desync_script;
		PhrasesPtrVector desync_pPhrases;
		format::Format desync_format = format::DetectFormat(desync_content);
		switch (desync_format)
		{
		case format::FMT_SRT:
			if (verbose) std::wclog << L"Формат: SRT" << std::endl;
			format::srt::Parse(desync_content, desync_phrases);
			for (format::srt::Phrases::size_type i = 0; i < desync_phrases.size(); ++i)
			{
				desync_pPhrases.push_back( &(desync_phrases[i]) );
			}
			break;

		case format::FMT_ASS:
			if (verbose) std::wclog << L"Формат: SSA/ASS" << std::endl;
			format::ass::Parse(desync_content, desync_script);
			for (format::ass::Events::size_type i = 0; i < desync_script.meta_events.events.size(); ++i)
			{
				desync_pPhrases.push_back( &(desync_script.meta_events.events[i]) );
			}
			break;

		default:
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Format not supported"))
				<< error_message(L"Формат не поддерживается")
			);
		}

		if (verbose) std::wclog << L"Группировка фраз" << std::endl;
		PhraseGroups desync_groups;
		GroupPhrases(desync_pPhrases, desync_groups);
		if (desync_groups.size() < 1)
		{
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(std::runtime_error("Phrase groups has not been formed"))
				<< error_message(L"Не сформировано ни одной группы")
			);
		}

		format::svg::OutputFormats output_formats;
		if (generate_svg)
		{
			output_formats.push_back( format::svg::OutputFormat(sync_groups, std::wstring(L"Synchronized"), std::wstring(L"#9BDFFF")) );
			output_formats.push_back( format::svg::OutputFormat(desync_groups, std::wstring(L"Desynchronized"), std::wstring(L"#FFE69E")) );
		}

		if (verbose) std::wclog << L"Поиск точек рассинхронизации" << std::endl;
		DesyncGroups desync_points;
		GetLCS(sync_groups, desync_groups, desync_points);
		if (desync_points.size() < 1)
		{
			std::wclog << L"Субтитры синхронны" << std::endl;
		}
		else
		{
			if (verbose) std::wclog << L"Точек рассинхронизации: " << desync_points.size() << std::endl;

			PhraseGroups sync_desync_groups, desync_desync_groups;
			for (DesyncGroups::iterator it = desync_points.begin(); it != desync_points.end(); ++it)
			{
				for (DesyncPositions::iterator dp = it->sync.begin(); dp != it->sync.end(); ++dp)
				{
					sync_desync_groups.push_back( sync_groups[*dp] );
				}
				for (DesyncPositions::iterator dp = it->desync.begin(); dp != it->desync.end(); ++dp)
				{
					desync_desync_groups.push_back( desync_groups[*dp] );
				}
			}

			if (verbose) std::wclog << L"Синхронизация" << std::endl;
			PhraseGroups result;
			Syncronize(sync_groups, desync_groups, desync_points, result);
			for (PhraseGroups::iterator it = result.begin(); it != result.end(); ++it)
			{
				it->applyShift();
			}

			if (generate_svg)
			{
				output_formats.push_back( format::svg::OutputFormat(result, std::wstring(L"Result"), std::wstring(L"#00A000")) );
				output_formats.push_back( format::svg::OutputFormat(sync_desync_groups, std::wstring(L"Valid"), std::wstring(L"#66FF9B")) );
				output_formats.push_back( format::svg::OutputFormat(desync_desync_groups, std::wstring(L"Invalid"), std::wstring(L"#FF7F7F")) );
			}

			if (verbose) std::wclog << L"Генерация синхронизированного скрипта" << std::endl;
			std::wstring out_content;
			switch (desync_format)
			{
			case format::FMT_SRT:
				format::srt::Generate(out_content, desync_phrases);
				break;

			case format::FMT_ASS:
				format::ass::Generate(out_content, desync_script);
				break;

			default:
				BOOST_THROW_EXCEPTION(
					boost::enable_error_info(std::runtime_error("Format not supported"))
					<< error_message(L"Формат не поддерживается")
				);
			}

			if (verbose) std::wclog << L"Вывод \"" << out_name.c_str() << L"\"" << std::endl;
			io::WriteFile(out_name, out_content);
		}

		// SVG
		if (generate_svg)
		{
			if (verbose) std::wclog << L"Генерация svg" << std::endl;
			std::wstring svg_content;
			format::svg::Generate(svg_content, output_formats);
			if (verbose) std::wclog << L"Вывод \"" << svg_name.c_str() << L"\"" << std::endl;
			io::WriteFile(svg_name, svg_content, false);
		}

		std::wclog << L"Готово!" << std::endl;
	}
	catch (const std::exception& e)
	{
		std::wcerr << L"Ошибка: ";
		if ( std::wstring const* info = boost::get_error_info<error_message>(e) )
		{
            std::wcerr << (*info) << std::endl;
        }
		else
		{
			std::wcerr << e.what() << std::endl;
		}
	}

#if defined _DEBUG && defined _MSC_VER && _MSC_VER >= 1600
	// В MSVC 2010 при запуске под отладчиком пауза больше не добавляется автоматически
	system("pause");
#endif

	// Не учтён выход с ошибкой выше
	return EXIT_SUCCESS;
}

void PrintHelp(char exec_name[])
{
	std::wcout << L"Re_Sync: подгонка тайминга по подходящему варианту субтитров.\n"
		L"Copyright (C) 2011  Ефремов А.В. <duxus@yandex.ru>\n"
		L"Программа распространяется бесплатно на условиях лицензии GPLv3.\n"
		L"\n"
		L"Использование: " << exec_name << L" [ключи] -s <файл> -d <файл> -o <файл>\n"
		L"Ключи:\n"
		L"  -s, --sync=<файл>       Синхронизированный скрипт\n"
		L"  -d, --desync=<файл>     Рассинхронизированный скрипт\n"
		L"  -o, --output=<файл>     Выходной скрипт\n"
		L"  -g, --graph=[файл]      Вывести внутреннее представление в виде графика\n"
		L"                          в формате SVG. Имя файла по умолчанию - \"graph.svg\".\n"
		L"\n"
		L"  --min-duration=<число>  Минимальная продолжительность фразы. Более короткие\n"
		L"                          фразы при синхронизации игнорируются.\n"
		L"                          По умолчанию 500 мс.\n"
		L"  --max-offset=<число>    Максимально допустимый отступ между фразами одной\n"
		L"                          группы. По умолчанию 5000 мс.\n"
		L"  --max-desync=<число>    Максимально допустимое расхождение фраз, при котором\n"
		L"                          они считаются синхронными. По умолчанию 200 мс.\n"
		L"  --max-shift=<число>     Максимально допустимое время, на которое могут быть\n"
		L"                          сдвинуты фразы. По умолчанию 15000 мс.\n"
		L"  --skip-lyrics           Пробовать пропускать открывающую и закрывающую песни\n"
		L"  --no-skip               Не применять фильтры комментариев и песен\n"
		L"  --allow-overlap         Разрешить перекрытие групп\n"
		L"\n"
		L"  -v, --verbose           Выводить подробности\n"
		L"  -h, --help              Вывести эту справку" << std::endl;
}
