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

#include <locale>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <boost/fusion/include/adapt_struct.hpp>

#include "exception.h"
#include "format.h"
#include "grammar.h"


BOOST_FUSION_ADAPT_STRUCT(
	format::srt::Phrase,
	(unsigned int, begin)
	(unsigned int, end)
	(boost::optional<std::wstring>, format)
	(std::wstring, text)
)

BOOST_FUSION_ADAPT_STRUCT(
	format::ass::Script,
	(std::wstring, head)
	(format::ass::MetaEvents, meta_events)
	(boost::optional<std::wstring>, tail)
)

BOOST_FUSION_ADAPT_STRUCT(
	format::ass::MetaEvents,
	(std::wstring, head)
	(format::ass::Events, events)
)

BOOST_FUSION_ADAPT_STRUCT(
	format::ass::Event,
	(std::wstring, layer)
	(unsigned int, begin)
	(unsigned int, end)
	(std::wstring, text)
	(boost::optional<std::wstring>, comments)
)


namespace format
{
	/*************************************************/
	/*   Функция определение формата по расширению   */
	/*************************************************/
	Format DetectFormat(const std::wstring& content)
	{
		if ( content.find(L"[Events]") != std::wstring::npos )
		{
			return FMT_ASS;
		}
		if ( content.find(L"-->") != std::wstring::npos )
		{
			return FMT_SRT;
		}
		return FMT_UNKNOWN;
	}

	/***********/
	/*   SRT   */
	/***********/
	namespace srt
	{
		/********************/
		/*   Разбор файла   */
		/********************/
		void Parse(std::wstring& input, Phrases& phrases)
		{
			namespace qi = boost::spirit::qi;
			namespace wide = boost::spirit::standard_wide;

			std::wstring::iterator begin = input.begin();
			std::wstring::iterator end = input.end();

			parser<std::wstring::iterator> parser;
			if (!qi::phrase_parse(begin, end, parser, wide::space, phrases) || begin != end)
			{
				BOOST_THROW_EXCEPTION(
					boost::enable_error_info(std::runtime_error("Parsing failed"))
					<< error_message(L"Ошибка разбора")
				);
			}
		}

		/*******************/
		/*   Вывод файла   */
		/*******************/
		void Generate(std::wstring& output, const Phrases& phrases)
		{
			namespace karma = boost::spirit::karma;
			typedef std::back_insert_iterator<std::wstring> sink_type;

			sink_type sink(output);

			generator<sink_type> generator;
			if (!karma::generate(sink, generator, phrases))
			{
				BOOST_THROW_EXCEPTION(
					boost::enable_error_info(std::runtime_error("Generating failed"))
					<< error_message(L"Ошибка генерации")
				);
			}
		}
	}

	namespace ass
	{
		/********************/
		/*   Разбор файла   */
		/********************/
		void Parse(std::wstring& input, Script& script)
		{
			namespace qi = boost::spirit::qi;
			namespace wide = boost::spirit::standard_wide;

			std::wstring::iterator begin = input.begin();
			std::wstring::iterator end = input.end();

			parser<std::wstring::iterator> parser;
			if (!qi::phrase_parse(begin, end, parser, wide::space, script) || begin != end)
			{
				BOOST_THROW_EXCEPTION(
					boost::enable_error_info(std::runtime_error("Parsing failed"))
					<< error_message(L"Ошибка разбора")
				);
			}
		}

		/*******************/
		/*   Вывод файла   */
		/*******************/
		void Generate(std::wstring& output, const Script& script)
		{
			namespace karma = boost::spirit::karma;
			typedef std::back_insert_iterator<std::wstring> sink_type;

			sink_type sink(output);

			generator<sink_type> generator;
			if (!karma::generate(sink, generator, script))
			{
				BOOST_THROW_EXCEPTION(
					boost::enable_error_info(std::runtime_error("Generating failed"))
					<< error_message(L"Ошибка генерации")
				);
			}
		}
	}

	/***********/
	/*   SVG   */
	/***********/
	namespace svg
	{
		OutputFormat::OutputFormat(const PhraseGroups& phrase_groups, const std::wstring& name, const std::wstring& color)
		{
			this->name = name;
			this->color = color;
			this->phrase_groups = phrase_groups; 
		}

		/*******************/
		/*   Вывод файла   */
		/*******************/
		void Generate(std::wstring& output, OutputFormats& output_formats)
		{
			const unsigned int POS_INC = 1, DIVIDER = 500, LINE_HEIGHT = 50;
			unsigned int max_width = 0, svg_width, svg_height;

			for (OutputFormats::iterator fmt = output_formats.begin(); fmt != output_formats.end(); ++fmt)
			{
				if (fmt->phrase_groups.back().getEnd() > max_width)
				{
					max_width = fmt->phrase_groups.back().getEnd();
				}
			}
			svg_width = max_width / DIVIDER;
			svg_height = LINE_HEIGHT * POS_INC * output_formats.size();

			std::wostringstream woss;
			woss.imbue(std::locale::classic());
			woss.fill(L'0');
			woss << L"<?xml version=\"1.0\" standalone=\"yes\"?>" << std::endl;
			woss << L"<svg version = \"1.1\" baseProfile=\"full\" xmlns=\"http://www.w3.org/2000/svg\" width=\""
				<< svg_width << L"\" height=\"" << svg_height << L"\">" << std::endl;

			unsigned int pos = 0, count, x, y;
			for (OutputFormats::iterator fmt = output_formats.begin(); fmt != output_formats.end(); ++fmt)
			{
				count = 1;
				y = LINE_HEIGHT * pos;
				for (PhraseGroups::iterator pg = fmt->phrase_groups.begin(); pg != fmt->phrase_groups.end(); ++pg)
				{
					x = pg->getBegin() / DIVIDER;

					woss << L"<rect x=\"" << x << L"px\" y=\""
						<< y << L"px\" width=\"" << ((pg->getEnd() - pg->getBegin()) / DIVIDER)
						<< L"px\" height=\"" << LINE_HEIGHT << L"px\" fill=\"" << fmt->color << L"\" />" << std::endl;

					woss << L"<text x=\"" << (x + 2) << L"\" y=\"" << (y + 10)
						<< L"\" font-family=\"Verdana\" font-size=\"10\" font-weight=\"bold\" fill=\"black\">"
						<< count << L"</text>" << std::endl;

					woss << L"<text x=\"" << (x + 2) << L"\" y=\"" << (y + 20)
						<< L"\" font-family=\"Verdana\" font-size=\"10\" fill=\"black\">"
						<< (pg->getBegin() / 60000u) << L':'
						<< std::setw(2) << ((pg->getBegin() % 60000u) / 1000u) << L'.'
						<< std::setw(3) << (pg->getBegin() % 1000u)
						<< L"</text>" << std::endl;

					woss << L"<text x=\"" << (x + 2) << L"\" y=\"" << (y + 30)
						<< L"\" font-family=\"Verdana\" font-size=\"10\" font-style=\"italic\" fill=\"black\">"
						<< (pg->getOffset() / 60000u) << L':'
						<< std::setw(2) << ((pg->getOffset() % 60000u) / 1000u) << L'.'
						<< std::setw(3) << (pg->getOffset() % 1000u)
						<< L"</text>" << std::endl;

					++count;
				}

				woss << L"<text x=\"5\" y=\"" << (y + LINE_HEIGHT - 5)
					<< L"\" font-family=\"Verdana\" font-size=\"10\" font-weight=\"bold\" fill=\"black\">"
					<< fmt->name << L"</text>" << std::endl;

				pos += POS_INC;
			}

			woss << L"</svg>" << std::endl;
			woss.str().swap(output);
		}
	}
}
