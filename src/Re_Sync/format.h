#pragma once

#include <string>

#include "structure.h"


namespace format
{
	enum Format {FMT_UNKNOWN, FMT_SRT, FMT_ASS};

	Format DetectFormat(const std::wstring& content);

	namespace srt
	{
		void Parse(std::wstring& input, Phrases& phrases);
		void Generate(std::wstring& output, const Phrases& phrases);
	}

	namespace ass
	{
		void Parse(std::wstring& input, Script& script);
		void Generate(std::wstring& output, const Script& script);
	}

	namespace svg
	{
		class OutputFormat
		{
		public:
			OutputFormat(const PhraseGroups& phrase_groups, const std::wstring& name, const std::wstring& color);

			std::wstring name, color;
			PhraseGroups phrase_groups;
		};
		typedef std::list<OutputFormat> OutputFormats;

		void Generate(std::wstring& output, OutputFormats& output_formats);
	}
}
