#pragma once

#include <string>
#include <vector>
#include <list>

#include <boost/optional.hpp>
#include <boost/fusion/adapted/struct/detail/extension.hpp>


/***************************/
/*   Базовый класс фразы   */
/***************************/
struct Phrase
{
	unsigned int begin, end;
	std::wstring text;
};
typedef std::vector<Phrase*> PhrasesPtrVector;
typedef std::list<Phrase*> PhrasesPtrList;

namespace format
{
	namespace srt
	{
		/***********************/
		/*   Класс фразы SRT   */
		/***********************/
		struct Phrase : ::Phrase
		{
			boost::optional<std::wstring> format;
		};
		typedef std::vector<Phrase> Phrases;
	}

	namespace ass
	{
		/*********************************/
		/*   Класс события скрипта ASS   */
		/*********************************/
		struct Event : ::Phrase
		{
			std::wstring layer;
			boost::optional<std::wstring> comments;
		};
		typedef std::vector<Event> Events;

		/*********************************/
		/*   Класс событий скрипта ASS   */
		/*********************************/
		struct MetaEvents
		{
			std::wstring head;
			Events events;
		};

		/*************************/
		/*   Класс скрипта ASS   */
		/*************************/
		struct Script
		{
			std::wstring head;
			MetaEvents meta_events;
			boost::optional<std::wstring> tail;
		};
	}
}

/*************************/
/*   Класс группы фраз   */
/*************************/
class PhraseGroup
{
	unsigned int _begin, _end, _offset;
	int _shift;
	PhrasesPtrList _phrases;

public:
	PhraseGroup(unsigned int begin, unsigned int end, unsigned int offset, PhrasesPtrList& phrases)
		: _begin(begin), _end(end), _offset(offset), _shift(0), _phrases(phrases) {};

	unsigned int getBegin();
	unsigned int getEnd();
	unsigned int getOffset();
	void setShift(int shift);
	void applyShift();
};

typedef std::vector<PhraseGroup> PhraseGroups;
