#pragma once

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include "structure.h"


namespace format
{
	/***********/
	/*   SRT   */
	/***********/
	namespace srt
	{
		namespace qi = boost::spirit::qi;
		namespace karma = boost::spirit::karma;
		namespace wide = boost::spirit::standard_wide;

		/**************/
		/*   Парсер   */
		/**************/
		template <typename Iterator>
		struct parser : qi::grammar<Iterator, Phrases(), wide::space_type>
		{
			parser() : parser::base_type(srt_file)
			{
				using qi::_val;
				using qi::_1;
				using qi::_2;
				using qi::_3;
				using qi::_4;
				using qi::_a;
			
				srt_file = +phrase;
				phrase = qi::skip(wide::blank)[qi::omit[qi::uint_] >> qi::eol
					>> timestamp >> L"-->" >> timestamp >> -(qi::lexeme[+(wide::char_ - qi::eol)]) >> qi::eol
					>> text >> (qi::eol | qi::eoi)];
				timestamp = ( qi::uint_parser<unsigned int, 10, 2, 2>() >> L':'
					>> qi::uint_parser<unsigned int, 10, 2, 2>() >> L':'
					>> qi::uint_parser<unsigned int, 10, 2, 2>() >> L','
					>> qi::uint_parser<unsigned int, 10, 3, 3>() )[_val = ((_1 * 60u + _2) * 60u + _3) * 1000u + _4];
				text = qi::raw[+(wide::char_ - qi::eol)[_val += _1] >> *(qi::eol[_a = L"\r\n"]
					>> +(wide::char_ - qi::eol)[_a += _1])[_val += _a]] >> (qi::eol | qi::eoi);
			}

			qi::rule<Iterator, Phrases(), wide::space_type> srt_file;
			qi::rule<Iterator, Phrase()> phrase;
			qi::rule<Iterator, unsigned int()> timestamp;
			qi::rule<Iterator, std::wstring(), qi::locals<std::wstring> > text;
		};

		/*****************/
		/*   Генератор   */
		/*****************/
		template <typename OutputIterator>
		struct generator : karma::grammar<OutputIterator, Phrases()>
		{
			generator() : generator::base_type(srt_file)
			{
				using karma::_val;
				using karma::_1;
				using karma::_a;
				using karma::_r1;
				using boost::phoenix::ref;

				count = 0u;

				srt_file = +phrase;
				phrase = number(++ref(count)) << L"\r\n"
					<< timestamp << L" --> " << timestamp << -(karma::lit(L"  ") << wide::string)
					<< L"\r\n" << text << L"\r\n\r\n";
				number = karma::uint_(_r1);
				timestamp = (karma::eps((_a = _val / 3600000u) < 10u) << L'0' << karma::uint_[_1 = _a] | karma::uint_[_1 = _a]) << L':'
					<< (karma::eps((_a = (_val % 3600000u) / 60000u) < 10u) << L'0' << karma::uint_[_1 = _a] | karma::uint_[_1 = _a]) << L':'
					<< (karma::eps((_a = (_val % 60000u) / 1000u) < 10u) << L'0' << karma::uint_[_1 = _a] | karma::uint_[_1 = _a]) << L',' 
					<< (karma::eps((_a = _val % 1000u) < 10u) << L"00" << karma::uint_[_1 = _a] | karma::eps(_a < 100u) << L'0' << karma::uint_[_1 = _a] | karma::uint_[_1 = _a]);
				text = wide::string;
			}

			karma::rule<OutputIterator, Phrases()> srt_file;
			karma::rule<OutputIterator, Phrase()> phrase;
			karma::rule<OutputIterator, void(unsigned int)> number;
			karma::rule<OutputIterator, unsigned int(), karma::locals<unsigned int> > timestamp;
			karma::rule<OutputIterator, std::wstring()> text;

			unsigned int count;
		};
	}

	namespace ass
	{
		namespace qi = boost::spirit::qi;
		namespace karma = boost::spirit::karma;
		namespace wide = boost::spirit::standard_wide;

		/**************/
		/*   Парсер   */
		/**************/
		template <typename Iterator>
		struct parser : qi::grammar<Iterator, Script(), wide::space_type>
		{
			parser() : parser::base_type(ass_file)
			{
				using qi::_val;
				using qi::_1;
				using qi::_2;
				using qi::_3;
				using qi::_4;
				
				ass_file = head >> events >> -(qi::raw[qi::lexeme[+wide::char_]]);
				head = qi::raw[+(!qi::lit(L"[Events]") >> wide::char_)];
				events = qi::raw[+(!(qi::lit(L"Dialogue:") | L"[Fonts]" | L"[Graphics]") >> wide::char_)] >> +event_;
				event_ = qi::skip(wide::blank)[qi::lit(L"Dialogue:") >> qi::raw[qi::lexeme[+(~wide::char_(L',') - qi::eol)]]
					>> L',' >> timestamp >> L',' >> timestamp >> L','
					>> qi::raw[qi::lexeme[+(wide::char_ - qi::eol)]]
					>> -(qi::raw[qi::lexeme[+(!(qi::lit(L"Dialogue:") | L"[Fonts]" | L"[Graphics]") >> wide::char_)]])];
				timestamp = ( qi::uint_parser<unsigned int, 10, 1, 1>() >> L':'
					>> qi::uint_parser<unsigned int, 10, 2, 2>() >> L':'
					>> qi::uint_parser<unsigned int, 10, 2, 2>() >> L'.'
					>> qi::uint_parser<unsigned int, 10, 2, 2>() )[_val = ((_1 * 60u + _2) * 60u + _3) * 1000u + _4 * 10u];
			}

			qi::rule<Iterator, Script(), wide::space_type> ass_file;
			qi::rule<Iterator, std::wstring()> head;
			qi::rule<Iterator, MetaEvents()> events;
			qi::rule<Iterator, Event()> event_;
			qi::rule<Iterator, unsigned int()> timestamp;
		};

		/*****************/
		/*   Генератор   */
		/*****************/
		template <typename OutputIterator>
		struct generator : karma::grammar<OutputIterator, Script()>
		{
			generator() : generator::base_type(ass_file)
			{
				using karma::_val;
				using karma::_1;
				using karma::_a;

				ass_file = head << events << -wide::string;
				head = wide::string;
				events = wide::string << +event_;
				event_ = karma::lit(L"Dialogue: ") << wide::string
					<< L',' << timestamp << L',' << timestamp << L',' << wide::string << -wide::string;
				timestamp = karma::uint_[_1 = _val / 3600000u] << L':'
					<< (karma::eps((_a = (_val % 3600000u) / 60000u) < 10u) << L'0' << karma::uint_[_1 = _a] | karma::uint_[_1 = _a]) << L':'
					<< (karma::eps((_a = (_val % 60000u) / 1000u) < 10u) << L'0' << karma::uint_[_1 = _a] | karma::uint_[_1 = _a]) << L'.' 
					<< (karma::eps((_a = (_val % 1000u) / 10u) < 10u) << L'0' << karma::uint_[_1 = _a] | karma::uint_[_1 = _a]);
			}

			karma::rule<OutputIterator, Script()> ass_file;
			karma::rule<OutputIterator, std::wstring()> head;
			karma::rule<OutputIterator, MetaEvents()> events;
			karma::rule<OutputIterator, Event()> event_;
			karma::rule<OutputIterator, unsigned int(), karma::locals<unsigned int> > timestamp;
		};
	}
}
