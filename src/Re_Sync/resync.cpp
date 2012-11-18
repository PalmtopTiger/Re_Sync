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

#include <cstdlib>
#include <algorithm>

#if defined _MSC_VER && _MSC_VER >= 1600
# include <regex>
using std::wregex;
namespace regex_constants = std::regex_constants;
#else
# include <boost/tr1/regex.hpp>
using boost::wregex;
namespace regex_constants = boost::regex_constants;
#endif

#include "resync.h"


bool PhrasePtrCmp (const Phrase* const first , const Phrase* const second)
{
	if (first->begin == second->begin)
	{
		return first->end < second->end;
	}
	else
	{
		return first->begin < second->begin;
	}
}

/***************************************/
/*   Фильтр фраз от авторов перевода   */
/***************************************/
inline bool IsCommentaryPhrase(const std::wstring& text)
{
	if (NO_SKIP) return false;

	static bool ret;

	static wregex pattern(
		L"(translat(ed?|or|ion)|(tim|typesett|encod|styl)(e[dr]|ing)|karaoke|note|QC|" // Английские
		L"перевод|(тайм|стайл)инг|тайпсет|(е|э)нкод|(кодирован|оформлен)ие|редак(тор|тирование|тура|ция)|караоке|коммент|" // Русские
		L"title|comment|sign|logo|insert|copyright|name|credit|" // Названия стилей
		L"\\\\pos|^\\[(\\s|\\S)+\\]$)",
		regex_constants::optimize | regex_constants::icase); // Комментарии

	ret = regex_search(text, pattern, regex_constants::format_first_only);

	if (SKIP_LYRICS && !ret)
	{
		static wregex pattern_lyrics(L"(op|ed)[ _,-]|opening|ending", regex_constants::optimize | regex_constants::icase);

		ret = regex_search(text, pattern_lyrics, regex_constants::format_first_only);
	}

	return ret;
}

/*********************************/
/*   Объединение фраз в группы   */
/*********************************/
void GroupPhrases(PhrasesPtrVector& pPhrases, PhraseGroups& groups)
{
	// Сортировка по времени
	sort(pPhrases.begin(), pPhrases.end(), PhrasePtrCmp);

	PhrasesPtrList pCurAccum, pDropAccum;
	int firstDropOffset = 0, tempOffset;
	unsigned int prevPhraseEnd = 0, curBegin = 0, curEnd = 0, curOffset = 0;
	bool bNoGroups = true;
	for (PhrasesPtrVector::iterator it = pPhrases.begin(); it != pPhrases.end(); ++it)
	{
		// Уже есть отброшенные и новая фраза отстоит от предыдущей дальше первой
		if ( !pDropAccum.empty() && static_cast<int>((*it)->begin) - static_cast<int>(prevPhraseEnd) > firstDropOffset && !bNoGroups )
		{
			// Добавить в текущую группу
			pCurAccum.splice(pCurAccum.end(), pDropAccum);
		}

		// Отбрасываем короткие фразы или комментарии переводчиков
		if ( static_cast<int>((*it)->end) - static_cast<int>((*it)->begin) < MIN_DURATION || IsCommentaryPhrase((*it)->text) )
		{
			// У первой фразы нет отступа
			if ( prevPhraseEnd == 0 )
			{
				firstDropOffset = 0;
			}
			else
			{
				firstDropOffset = static_cast<int>((*it)->begin) - static_cast<int>(prevPhraseEnd);
			}

			pDropAccum.push_back(*it);
		}
		else // Подходящая фраза
		{
			if ( static_cast<int>((*it)->begin) - static_cast<int>(prevPhraseEnd) > MAX_OFFSET || bNoGroups )
			{
				if (!bNoGroups)
				{
					groups.push_back( PhraseGroup(curBegin, curEnd, curOffset, pCurAccum) );
					pCurAccum.clear();
				}

				// Новая группа
				// Считаем отступы между началом групп
				tempOffset = static_cast<int>((*it)->begin) - static_cast<int>(curBegin);
				// Считаем отступы между группами
				//tempOffset = static_cast<int>((*it)->begin) - static_cast<int>(curEnd);
				curOffset = tempOffset < 0 ? 0u : static_cast<unsigned int>(tempOffset);

				curBegin = (*it)->begin;

				bNoGroups = false;
			}
			
			// Скинуть все накопленные, если есть
			if ( !pDropAccum.empty() )
			{
				pCurAccum.splice(pCurAccum.end(), pDropAccum);
			}
			
			curEnd = (*it)->end;
			pCurAccum.push_back(*it);
		}
		
		prevPhraseEnd = (*it)->end;
	}
	// Скинуть оставшиеся
	if ( !pDropAccum.empty() )
	{
		pCurAccum.splice(pCurAccum.end(), pDropAccum);
	}
	if ( !pCurAccum.empty() && !bNoGroups )
	{
		groups.push_back( PhraseGroup(curBegin, curEnd, curOffset, pCurAccum) );
	}

	// Отладка
	/*
	size_t debug_phrases_count = 0;
	for (PhraseGroups::iterator it = groups.begin(); it != groups.end(); ++it)
	{
		std::wcout << L"Группа из " << it->_phrases.size() << std::endl;
		debug_phrases_count += it->_phrases.size();
	}
	std::wcout << L"Фраз в скрипте: " << pPhrases.size() << std::endl <<
		L"Фраз в группах: " << debug_phrases_count << std::endl <<
		L"Групп: " << groups.size() << std::endl << std::endl;
	*/
}

#if 1
/*********************************************************/
/*   Нахождение наибольшей общей подпоследовательности   */
/*********************************************************/
void GetLCS(PhraseGroups& sync, PhraseGroups& desync, DesyncGroups& result)
{
	// Построение таблицы
	std::vector< std::vector<unsigned int> > max_len(sync.size() + 1);
	for (size_t i = 0; i <= sync.size(); ++i)
	{
		max_len[i].resize(desync.size() + 1);
		for (size_t j = 0; j <= desync.size(); ++j)
		{
			max_len[i][j] = 1;
		}
	}

	// Заполнение таблицы
	{
		int i, j;
		for (i = static_cast<int>(sync.size()) - 1; i >= 0; --i)
		{
			for (j = static_cast<int>(desync.size()) - 1; j >= 0; --j)
			{
				if ( abs(static_cast<int>(sync[i].getOffset()) - static_cast<int>(desync[j].getOffset())) <= MAX_DESYNC )
				{
					max_len[i][j] = max_len[i+1][j+1] + 1;
				}
				else
				{
					max_len[i][j] = std::max(max_len[i+1][j], max_len[i][j+1]);
				}
			}
		}
	}

	// Нахождение наибольшей общей подпоследовательности
	DesyncPositions syncAccum, desyncAccum;
	size_t i = 0, j = 0;
	while (max_len[i][j] != 0 && i < sync.size() && j < desync.size())
	{
		if ( abs(static_cast<int>(sync[i].getOffset()) - static_cast<int>(desync[j].getOffset())) <= MAX_DESYNC )
		{
			if (!syncAccum.empty() && !desyncAccum.empty())
			{
				result.push_back( DesyncGroup(syncAccum, desyncAccum) );
			}
			syncAccum.clear();
			desyncAccum.clear();
			++i;
			++j;
		}
		else
		{
			if (max_len[i][j] == max_len[i+1][j])
			{
				syncAccum.push_back(i);
				++i;
			}
			else
			{
				desyncAccum.push_back(j);
				++j;
			}
		}
	}
}

#else
/*******************************************************************************************/
/*   Нахождение наибольшей общей подпоследовательности (обратный способ, устаревший код)   */
/*******************************************************************************************/
void GetLCS(PhraseGroups& sync, PhraseGroups& desync, DesyncGroups& result)
{
	// Построение таблицы
	std::vector< std::vector<unsigned int> > max_len(desync.size() + 1);
	for (size_t i = 0; i <= desync.size(); ++i)
	{
		max_len[i].resize(sync.size() + 1);
		for (size_t j = 0; j <= sync.size(); ++j)
		{
			max_len[i][j] = 1;
		}
	}

	// Заполнение таблицы
	{
		int i, j;
		for (i = static_cast<int>(desync.size()) - 1; i >= 0; --i)
		{
			for (j = static_cast<int>(sync.size()) - 1; j >= 0; --j)
			{
				if ( abs(static_cast<int>(sync[j].getOffset()) - static_cast<int>(desync[i].getOffset())) <= MAX_DESYNC )
				{
					max_len[i][j] = max_len[i+1][j+1] + 1;
				}
				else
				{
					max_len[i][j] = std::max(max_len[i+1][j], max_len[i][j+1]);
				}
			}
		}
	}

	// Нахождение наибольшей общей подпоследовательности
	DesyncPositions syncAccum, desyncAccum;
	size_t i = 0, j = 0;
	while (max_len[i][j] != 0 && i < desync.size() && j < sync.size())
	{
		if ( abs(static_cast<int>(sync[j].getOffset()) - static_cast<int>(desync[i].getOffset())) <= MAX_DESYNC )
		{
			if (!syncAccum.empty() && !desyncAccum.empty())
			{
				result.push_back( DesyncGroup(syncAccum, desyncAccum) );
			}
			syncAccum.clear();
			desyncAccum.clear();
			++i;
			++j;
		}
		else
		{
			if (max_len[i][j] == max_len[i+1][j])
			{
				desyncAccum.push_back(i);
				++i;
			}
			else
			{
				syncAccum.push_back(j);
				++j;
			}
		}
	}
}
#endif

/******************************************/
/* Нахождение числа рассинхронизированных */
/******************************************/
unsigned int CountSyncronized(PhraseGroups& sync, PhraseGroups& desync)
{
	unsigned int count = 0;
	size_t i = 0, j = 0;
	while (i < sync.size() && j < desync.size())
	{
		if ( abs(static_cast<int>(sync[i].getBegin()) - static_cast<int>(desync[j].getBegin())) <= MAX_DESYNC )
		{
			++count;
			i++;
			j++;
		}
		else if ( sync[i].getBegin() < desync[j].getBegin() )
		{
			i++;
		}
		else
		{
			j++;
		}
	}

	return count;
}

/*********************/
/*   Синхронизация   */
/*********************/
void Syncronize(PhraseGroups& sync, PhraseGroups& desync, DesyncGroups& desync_points, PhraseGroups& result)
{
	// Полная синхронизация
	if (desync_points.size() < 1)
	{
		return;
	}
	
	size_t i, j, k;
	// Первые синхронны
	if (desync_points[0].desync[0] > 0)
	{
		for (i = 0; i < desync_points[0].desync[0]; ++i)
		{
			result.push_back(desync[i]);
		}
	}

	size_t until_pos_sync, until_pos_desync, pos;
	unsigned int sync_count, best_sync_count;
	int shift, best_shift;
	PhraseGroups temp_sync, temp_desync;
	unsigned int prev_end = 0;
	for (i = 0; i < desync_points.size(); ++i)
	{
		DesyncPositions& sync_pos_i = desync_points[i].sync;
		DesyncPositions& desync_pos_i = desync_points[i].desync;
		
		// Последняя группа или нет
		if (i < desync_points.size() - 1)
		{
			until_pos_sync = desync_points[i + 1].sync[0];
			until_pos_desync = desync_points[i + 1].desync[0];
		}
		else
		{
			until_pos_sync = sync.size();
			until_pos_desync = desync.size();
		}
		
		best_sync_count = 0;
		best_shift = 0;
		for (j = 0; j < sync_pos_i.size(); ++j)
		{
			for (k = 0; k < desync_pos_i.size(); ++k)
			{
				shift = static_cast<int>(sync[sync_pos_i[j]].getBegin()) - static_cast<int>(desync[desync_pos_i[k]].getBegin());
				if (abs(shift) > MAX_SHIFT) continue;
				
				temp_desync.clear();
				for (pos = desync_pos_i[0]; pos < until_pos_desync; ++pos)
				{
					temp_desync.push_back( desync[pos] );
					temp_desync.back().setShift(shift);
				}
				// Нельзя залазить на предыдущую группу
				if (!ALLOW_OVERLAP && temp_desync.begin()->getBegin() < prev_end) continue;

				temp_sync.clear();
				for (pos = sync_pos_i[0]; pos < until_pos_sync; ++pos)
				{
					temp_sync.push_back( sync[pos] );
				}
				
				sync_count = CountSyncronized(temp_sync, temp_desync);
				if (sync_count > best_sync_count)
				{
					best_sync_count = sync_count;
					best_shift = shift;
				}
			}
		}

		// Перекидываем лучший результат
		for (pos = desync_pos_i[0]; pos < until_pos_desync; ++pos)
		{
			result.push_back( desync[pos] );
			if (best_shift != 0)
			{
				result.back().setShift(best_shift);
			}
		}
		prev_end = result.back().getEnd();
	}
}
