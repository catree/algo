#include "test_common.h"

/* Represent a TicTacToe board as a uint32_t. An X in a cell corresponds to the following bits being set/clear:
 *    0|1|2
 *    -+-+-
 *    3|4|5
 *    -+-+-
 *    6|7|8
 * An O in a cell corresponds to the same indices plus 9.
 */
typedef uint32_t TicTacToeState;
static int tttIsValid(TicTacToeState state)
{
	return (state < (1<<18)) && ( ((state>>9) & 0x1FF) & ((state>>0) & 0x1FF) ) == 0;
}
static char tttGetNextPlayer(const TicTacToeState state)
{
	return (__popcnt(state) % 2) ? 'O' : 'X';
}
static char tttGetCell(const TicTacToeState state, int cellIndex)
{
	return ((state>> 0) & (1<<cellIndex)) ? 'X' : (((state>>9) & (1<<cellIndex)) ? 'O' : ' ');
}
static TicTacToeState tttSetCell(const TicTacToeState state, uint32_t cellIndex, char value)
{
	assert(value == 'X' || value == 'x' || value == 'O' || value == 'o');
	assert(tttIsValid(state));
	assert(cellIndex<9);
	assert(tttGetCell(state, cellIndex) == ' '); /* cell must be empty */
	cellIndex += ((value == 'X' || value == 'x') ? 0 : 9);
	return state | (1<<cellIndex);
}
static char tttGetWinner(const TicTacToeState state)
{
	const uint32_t victoryMasks[8] = {
		0x007, /* 0,1,2 */
		0x038, /* 3,4,5 */
		0x1C0, /* 6,7,8 */
		0x049, /* 0,3,6 */
		0x092, /* 1,4,7 */
		0x124, /* 2,5,8 */
		0x111, /* 0,4,8 */
		0x054, /* 2,4,6 */
	};
	assert(tttIsValid(state));
	int iMask;
	for(iMask=0; iMask<8; ++iMask)
	{
		if ( ((state>>0) & victoryMasks[iMask]) == victoryMasks[iMask] )
			return 'X'; /* winner is X */
		if ( ((state>>9) & victoryMasks[iMask]) == victoryMasks[iMask] )
			return 'O'; /* winner is O */
	}
	return ' '; /* No winner */
}
static void tttPrint(const TicTacToeState state)
{
	printf("%c|%c|%c\n-+-+-\n", tttGetCell(state, 0), tttGetCell(state, 1), tttGetCell(state, 2));
	printf("%c|%c|%c\n-+-+-\n", tttGetCell(state, 3), tttGetCell(state, 4), tttGetCell(state, 5));
	printf("%c|%c|%c\n\n",      tttGetCell(state, 6), tttGetCell(state, 7), tttGetCell(state, 8));
}
typedef struct HashEntry_StateToInt
{
	TicTacToeState key;
	int32_t value;
	struct HashEntry_StateToInt *next;
} HashEntry_StateToInt;
typedef struct HashTable_StateToInt
{
	int binCount;
	int entryCapacity;
	int freeEntryIndex;
	HashEntry_StateToInt **bins;
	HashEntry_StateToInt *entryPool;
} HashTable_StateToInt;
static size_t hashBufferSize_StateToInt(int binCount, int entryCapacity)
{
	return entryCapacity*sizeof(HashEntry_StateToInt) + binCount*sizeof(HashEntry_StateToInt*) + sizeof(HashTable_StateToInt);
}
static HashTable_StateToInt *hashCreate_StateToInt(void *buffer, size_t bufferSize, int binCount, int entryCapacity)
{
	size_t expectedBufferSize = hashBufferSize_StateToInt(binCount, entryCapacity);
	if (bufferSize < expectedBufferSize)
		return NULL;
	uint8_t *bufferNext = (uint8_t*)buffer;

	HashEntry_StateToInt *entryPool = (HashEntry_StateToInt*)bufferNext;
	bufferNext += entryCapacity*sizeof(HashEntry_StateToInt);

	HashEntry_StateToInt **bins = (HashEntry_StateToInt**)bufferNext;
	bufferNext += binCount*sizeof(HashEntry_StateToInt*);
	int iBin;
	for(iBin=0; iBin<binCount; iBin+=1)
		bins[iBin] = NULL;

	HashTable_StateToInt *outTable = (HashTable_StateToInt*)bufferNext;
	bufferNext += sizeof(HashTable_StateToInt);
	assert( (size_t)(bufferNext-(uint8_t*)buffer) == expectedBufferSize );
	outTable->binCount = binCount;
	outTable->entryCapacity = entryCapacity;
	outTable->freeEntryIndex = 0;
	outTable->bins = bins;
	outTable->entryPool = entryPool;
	return outTable;
}
static HashEntry_StateToInt **hashGetEntryRef_StateToInt(HashTable_StateToInt *table, TicTacToeState key, int binIndex)
{
	assert(binIndex >= 0 && binIndex < table->binCount);
	HashEntry_StateToInt **ppEntry;
	for(ppEntry = &(table->bins[binIndex]);
		NULL != *ppEntry;
		ppEntry = &(*ppEntry)->next)
	{
		if (key == (*ppEntry)->key)
			return ppEntry;
	}
	return NULL;
}
static int32_t tttGetVertexId(HashTable_StateToInt *table, TicTacToeState state)
{
	int binIndex = state % (table->binCount); /* lamest hash function ever */
	HashEntry_StateToInt **ppEntry = hashGetEntryRef_StateToInt(table, state, binIndex);
	if (NULL == ppEntry)
		return -1; /* not found */
	return (*ppEntry)->value;
}
static void tttSetVertexId(HashTable_StateToInt *table, TicTacToeState state, int32_t vertexId)
{
	int binIndex = state % (table->binCount); /* lamest hash function ever */
	HashEntry_StateToInt **ppEntry = hashGetEntryRef_StateToInt(table, state, binIndex);
	if (NULL != ppEntry)
	{
		assert(0); /* Shouldn't be updating any table entries in this use case */
		(*ppEntry)->value = vertexId;
	}
	else
	{
		assert(table->freeEntryIndex < table->entryCapacity);
		HashEntry_StateToInt *newEntry = table->entryPool + table->freeEntryIndex;
		table->freeEntryIndex += 1;
		newEntry->key = state;
		newEntry->value = vertexId;
		newEntry->next = table->bins[binIndex];
		table->bins[binIndex] = newEntry;
	}
}

static void addMovesForState(AlgoGraph graph, HashTable_StateToInt *table, int32_t vertexId)
{
	TicTacToeState state;
	ALGO_VALIDATE( algoGraphGetVertexData(graph, vertexId, (AlgoData*)&state) );
	char player = tttGetNextPlayer(state);
	int iCell = 0;
	for(iCell = 0; iCell<9; ++iCell)
	{
		/** If iCell is not empty:
				continue
			nextState = setCell(iCell, player)
			recurse = (nextState is not a winning state)
			If a vertex does not exist for nextState:
				nextVertexId = addVertex(nextState)
				recurse = true
			Add an edge from vertexId to nextVertexId
			if recurse:
				addMovesForState(graph, nextVertexId, !player)
			*/
		if (tttGetCell(state, iCell) != ' ')
			continue;
		TicTacToeState nextState = tttSetCell(state, iCell, player);
		char nextStateWinner = tttGetWinner(nextState);
		int nextStateIsNew = 0;
		int32_t nextVertexId = tttGetVertexId(table, nextState);
		if (nextVertexId == -1)
		{
			ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromInt(nextState), &nextVertexId) );
			tttSetVertexId(table, nextState, nextVertexId);
			nextStateIsNew = 1;
		}
		ALGO_VALIDATE( algoGraphAddEdge(graph, vertexId, nextVertexId) );
		if (nextStateIsNew && (nextStateWinner == ' '))
		{
			addMovesForState(graph, table, nextVertexId);
		}
	}
}

static void dfsValidateVertex(AlgoGraph graph, int32_t vertexId)
{
	TicTacToeState state = 0;
	ALGO_VALIDATE( algoGraphGetVertexData(graph, vertexId, (AlgoData*)&state) );
	assert( tttIsValid(state) );
	char winner = tttGetWinner(state);
	int32_t degree = -1;
	ALGO_VALIDATE( algoGraphGetVertexDegree(graph, vertexId, &degree) );
	assert(degree <= 9);
	assert(winner == ' ' || degree == 0);
	int32_t edges[9];
	ALGO_VALIDATE( algoGraphGetVertexEdges(graph, vertexId, degree, edges) );
	char nextPlayer = tttGetNextPlayer(state);
	for(int iEdge=0; iEdge<degree; ++iEdge)
	{
		TicTacToeState nextState = 0;
		ALGO_VALIDATE( algoGraphGetVertexData(graph, edges[iEdge], (AlgoData*)&nextState) );
		char nextNextPlayer = tttGetNextPlayer(nextState);
		assert(nextPlayer != nextNextPlayer);
		(void)nextPlayer;
		(void)nextNextPlayer;
	}
}

int main()
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	AlgoGraph graph;
	void *graphBuffer = NULL;
	size_t graphBufferSize = 0;
#if 0 /* conservative estimates */
	const int32_t kVertexCapacity = 3*3*3 * 3*3*3 * 3*3*3; /* 9 cells, 3 possible values per cell = 3^9 states */
	const int32_t kEdgeCapacity   = 9*8*7*6*5*4*3*2*1; // 9 choices for the first move, 8 for the next, etc. This is conservative, but reasonable. */
#else /* exact values -- smaller due to reusing state vertices & victory terminating the tree. */
	const int32_t kVertexCapacity = 5478;
	const int32_t kEdgeCapacity   = 16167;
#endif
	ALGO_VALIDATE( algoGraphBufferSize(&graphBufferSize, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected) );
	graphBuffer = malloc(graphBufferSize);
	ALGO_VALIDATE( algoGraphCreate(&graph, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected, graphBuffer, graphBufferSize) );
	ALGO_VALIDATE( algoGraphValidate(graph) );

	size_t hashTableBufferSize = hashBufferSize_StateToInt(8192, kVertexCapacity);
	void *hashTableBuffer = malloc(hashTableBufferSize);
	HashTable_StateToInt *table = hashCreate_StateToInt(hashTableBuffer, hashTableBufferSize, 8192, kVertexCapacity);
	assert(table);

	TicTacToeState startState = 0;
	int32_t startVertexId = 0;
	ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromInt(startState), &startVertexId) );
	tttSetVertexId(table, startState, startVertexId);
	addMovesForState(graph, table, startVertexId);

	size_t dfsBufferSize = 0;
	ALGO_VALIDATE( algoGraphDfsBufferSize(&dfsBufferSize, graph) );
	void *dfsBuffer = malloc(dfsBufferSize);
	int32_t *vertexParents = malloc(kVertexCapacity*sizeof(int32_t));
	/** TODO:
		Add algoGraphSetEdgeData(), algoGraphGetEdgeData()
		Add void parameters to AlgoGraphProcessVertexFunc and AlgoGraphProcessEdgeFunc
		lateFunc:
			winner = tttGetWinner();
			if winner != ' ':
				assert(degree == 0)
				mark vert as a winner
				return
			else:
				for each edge:
					if edge destination's state is a winner:
						set edge data as a winner

		*/		
	ALGO_VALIDATE( algoGraphDfs(graph, startVertexId, vertexParents, kVertexCapacity,
		dfsValidateVertex, NULL, NULL, dfsBuffer, dfsBufferSize) );
	free(dfsBuffer);

#if 0 /* just some quicky hash table analysis */
	int iBin=0;
	float avgLength = 0;
	int maxLength = 0;
	for(iBin=0; iBin<table->binCount; ++iBin)
	{
		HashEntry_StateToInt *entry = table->bins[iBin];
		int length = 0;
		while(entry)
		{
			length += 1;
			entry = entry->next;
		}
		avgLength += length;
		if (length > maxLength)
			maxLength = length;
	}
	avgLength *= (1.0f / (float)(table->binCount));
#endif
	free(hashTableBuffer);
	free(graphBuffer);
}