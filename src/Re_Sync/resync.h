#pragma once

#include "structure.h"


/*****************************************************/
/*   Класс сгруппированных не совпавших групп фраз   */
/*****************************************************/
typedef std::vector<size_t> DesyncPositions;

class DesyncGroup
{
public:
	DesyncGroup(DesyncPositions& sync, DesyncPositions& desync) : sync(sync), desync(desync) {};
	DesyncPositions sync, desync;
};

typedef std::vector<DesyncGroup> DesyncGroups;


void GroupPhrases(PhrasesPtrVector& pPhrases, PhraseGroups& groups);
void GetLCS(PhraseGroups& sync, PhraseGroups& desync, DesyncGroups& result);
void Syncronize(PhraseGroups& sync, PhraseGroups& desync, DesyncGroups& desync_points, PhraseGroups& result);

extern int MIN_DURATION;
extern int MAX_OFFSET;
extern int MAX_DESYNC;
extern int MAX_SHIFT;
extern bool SKIP_LYRICS;
extern bool NO_SKIP;
extern bool ALLOW_OVERLAP;
