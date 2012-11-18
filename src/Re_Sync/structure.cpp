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

#include "structure.h"


unsigned int PhraseGroup::getBegin()
{
	int ret = static_cast<int>(_begin) + _shift;
	if (ret < 0) ret = 0;
	return static_cast<unsigned int>(ret);
}
	
unsigned int PhraseGroup::getEnd()
{
	int ret = static_cast<int>(_end) + _shift;
	if (ret < 0) ret = 0;
	return static_cast<unsigned int>(ret);
}

unsigned int PhraseGroup::getOffset()
{
	return _offset;
}
	
void PhraseGroup::setShift(int shift)
{
	_shift = shift;
}

void PhraseGroup::applyShift()
{
	int temp;
	Phrase* pPhrase;

	for (PhrasesPtrList::iterator it = _phrases.begin(); it != _phrases.end(); ++it)
	{
		pPhrase = (*it);

		temp = static_cast<int>(pPhrase->begin) + _shift;
		if (temp < 0) temp = 0;
		pPhrase->begin = static_cast<unsigned int>(temp);

		temp = static_cast<int>(pPhrase->end) + _shift;
		if (temp < 0) temp = 0;
		pPhrase->end = static_cast<unsigned int>(temp);
	}
}
