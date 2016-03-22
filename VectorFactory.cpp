#include <list>
#include <ext/hash_map>
#include <pthread.h>

#ifndef HASH_MAP_TOKENS
#define HASH_MAP_TOKENS hash_map<char*, int, hash<char*>, eqstr>
#endif

#define HASH_MAP_VECTOR hash_map<char*, DocumentVector*, hash<char*>, eqstr>
#define HASH_MAP_OCCURENCE_TABLE hash_map<char*, int, hash<char*>, eqstr>
#define HASH_MAP_PARSED_DOCS hash_map<char*, HASH_MAP_TOKENS, hash<char*>, eqstr>

extern char cwd[1024];

using namespace __gnu_cxx;
using namespace std;

class VectorFactory {

public:
	/*Changed method signature to accept number of threads*/
	HASH_MAP_VECTOR createVectors(Parser parser, list<char*> docs, int numberOfThreads);
	HASH_MAP_OCCURENCE_TABLE createCorpusOccurenceTable(HASH_MAP_PARSED_DOCS parsedDocs);
	/*Changed method signature to accept number of threads*/
	HASH_MAP_PARSED_DOCS parseDocs(Parser parser, list<char*> docs, int threads);
	void dumpDocs(Parser parser, HASH_MAP_PARSED_DOCS parsedDocs);
};

struct timeval parser_start;
struct timeval parser_end;
struct timeval corpus_start;
struct timeval corpus_end;
struct timeval normalize_start;
struct timeval normalize_end;


long calcDiffTime(struct timeval* strtTime, struct timeval* endTime)
{
	return(
			endTime->tv_sec*1000000 + endTime->tv_usec
			- strtTime->tv_sec*1000000 - strtTime->tv_usec
	);

}

/*
 * Structure to pass arguments to calcTfIdf method
 *
 */

struct calcTfidfArgs{
	HASH_MAP_PARSED_DOCS* parsedDocs;
	HASH_MAP_OCCURENCE_TABLE* corpusOccurences;
	HASH_MAP_VECTOR vectors;
	long* normalizetime;
	int threadNumber;
	int totalThreads;
};

/*
 * calcTfIdf method - Calculates the TfIdf for each of the documents
 *
 */

void *calcTfidf(void* arg){
	struct calcTfidfArgs* args = (struct calcTfidfArgs*)arg;
	HASH_MAP_PARSED_DOCS parsedDocs = *(args->parsedDocs);
	HASH_MAP_OCCURENCE_TABLE corpusOccurences = *(args->corpusOccurences);
	DocumentVector *dv = new DocumentVector();

	HASH_MAP_PARSED_DOCS::const_iterator it1;

	int threadNo = args->threadNumber;
	int totalThreads = args->totalThreads;


	HASH_MAP_TOKENS::const_iterator it2;
	int index=-1;

	for ( it1=parsedDocs.begin() ; it1 != parsedDocs.end(); it1++ )
	{
		index++;
		if(index%totalThreads!=threadNo)
			continue;
		char *file = (*it1).first;
		HASH_MAP_TOKENS freqTable = (*it1).second;

		DocumentVector *dv = new DocumentVector();

		HASH_MAP_TOKENS::const_iterator it2;
		for ( it2=freqTable.begin() ; it2 != freqTable.end(); it2++)
		{
			char *token = (*it2).first;
			int freq;
			freq= (int)((*it2).second);

			float tf = (float)freq;
			float docQuan = (float) parsedDocs.size();
			float corpOcc = (float)(corpusOccurences[token]);
			float idf = (float) (log(docQuan / corpOcc));

			float tfidf = tf * idf;

			dv->addElement(token, tfidf);
		}

		struct timeval normalize_start;
		gettimeofday(&normalize_start, NULL);
		args->vectors[file] = dv->normalize();
		struct timeval normalize_end;
		gettimeofday(&normalize_end, NULL);
		/*Update Normalization time for each thread*/
		*(args->normalizetime) += calcDiffTime(&normalize_start, &normalize_end);
	}
}

HASH_MAP_VECTOR VectorFactory::createVectors(Parser parser, list<char*> docs, int threads){

	HASH_MAP_VECTOR vectors;

	gettimeofday(&parser_start, NULL);
	HASH_MAP_PARSED_DOCS parsedDocs = parseDocs(parser, docs, threads);
	gettimeofday(&parser_end, NULL);
	long parsetime = calcDiffTime(&parser_start, &parser_end);
	printf("parsetime = %ld\n", parsetime);

#ifdef AFFIXES_ONLY
	VectorFactory v1;
	v1.dumpDocs(parser, parsedDocs);
	return(vectors);
#endif

	gettimeofday(&corpus_start, NULL);
	HASH_MAP_OCCURENCE_TABLE corpusOccurences = createCorpusOccurenceTable(parsedDocs);
	gettimeofday(&corpus_end, NULL);
	long corpustime = calcDiffTime(&corpus_start, &corpus_end);
	printf("corpustime = %ld\n", corpustime);

	long normalizetime = 0;
	struct calcTfidfArgs args[threads];
	int listSize = parsedDocs.size();
	/* If number of threads are more than number of documents
	 * Limit number of threads to number of documents (1 thread per document)
	 * */
	if (threads > listSize)
		threads = listSize;

	int threadNumber = 0;
	pthread_t calcTfidfThreads[threads];
	/**
	 * For Each Thread, prepare the argument array.
	 */
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		args[threadNumber].parsedDocs = &parsedDocs;
		args[threadNumber].corpusOccurences= &corpusOccurences;
		args[threadNumber].threadNumber = threadNumber;
		args[threadNumber].totalThreads = threads;
		args[threadNumber].normalizetime = &normalizetime;
	}
	/* call the pthread create to create a new thread and call calcTfidf for each of the threads*/
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		if(0!=pthread_create(&calcTfidfThreads[threadNumber],NULL,calcTfidf,&args[threadNumber])){
			printf("Error in creating thread. Program will exit now.");
			exit(1);
		}
	}
	/* wait for all the threads to join..*/
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		if(0!=pthread_join(calcTfidfThreads[threadNumber],NULL)){
			printf("Error in joining thread. Program will exit now.");
			exit(1);
		}
	}

	/*collect data from each of the threads*/
	HASH_MAP_VECTOR::iterator it1;
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		for ( it1=args[threadNumber].vectors.begin() ; it1 != args[threadNumber].vectors.end(); it1++ )
		{
			vectors[(*it1).first] = (*it1).second;
		}
	}

	printf("normalizetime = %ld\n", normalizetime);
	return(vectors);
}

/*
 * Argument structure for parseDocs.
 */

struct parseDocsArgs{
	list<char*> *docs;
	Parser* parser;
	HASH_MAP_PARSED_DOCS parsedDocs;
	int threadNumber;
	int totalThreads;
};

/**
 * Separate the parser.parse from parseDocs method.
 */
void *parseDocsWrapper(void* arg)
{
	struct parseDocsArgs* args = (struct parseDocsArgs*)arg;
	Parser p = *args->parser;
	list<char*>::iterator it1;
	list<char*> docs = *(args->docs);
	int threadNo = args->threadNumber;
	int totalThreads = args->totalThreads;
	int index=-1;

	for(it1 = docs.begin();it1!=docs.end();it1++){
		index++;
		if(index%totalThreads!=threadNo)
			continue;
		HASH_MAP_TOKENS freq;
		char *file = (char *) malloc(strlen(cwd) + strlen(*it1) + 1);
		strcpy(file, cwd);
		strcat(file, *it1);
		FILE* fp = fopen(file, "r");
		freq = p.parseFile(fp);
		fclose(fp);
		(args->parsedDocs)[*it1] = freq;
	}
	pthread_exit(NULL);
}

HASH_MAP_PARSED_DOCS VectorFactory::parseDocs(Parser parser, list<char*> docs, int threads) {
	HASH_MAP_PARSED_DOCS parsedDocs;


	pthread_t doc_threads[threads];
	int threadNumber;
	HASH_MAP_TOKENS freqTable[threads];
	struct parseDocsArgs args[threads];
	int listSize = docs.size();//78
	if (threads > listSize)
		threads = listSize;

	char* file[threads];
	int count = 0;
	/* Prepare arguments for each of the thread */
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		args[threadNumber].parser = &parser;
		args[threadNumber].docs = &docs;
		args[threadNumber].threadNumber = threadNumber;
		args[threadNumber].totalThreads = threads;
	}
	/* Create n threads, each with its own arguments*/
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		if(0!=pthread_create(&doc_threads[threadNumber],NULL,parseDocsWrapper,&args[threadNumber])){
			printf("Error in creating thread. Program will exit now.");
			exit(1);
		}
	}
	/*Wait for all the threads to Join*/
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		if(0!=pthread_join(doc_threads[threadNumber],NULL)){
			printf("Error in joining thread. Program will exit now.");
			exit(1);
		}
	}
	/*Combine the data in single map*/
	for(threadNumber=0;threadNumber<threads;threadNumber++)
	{
		parsedDocs.insert(args[threadNumber].parsedDocs.begin(), args[threadNumber].parsedDocs.end());
	}
	return(parsedDocs);
}

void VectorFactory::dumpDocs(Parser parser, HASH_MAP_PARSED_DOCS parsedDocs){
	extern char cwd[1024];
	HASH_MAP_PARSED_DOCS::const_iterator it1;

	for ( it1=parsedDocs.begin() ; it1 != parsedDocs.end(); it1++ )
	{
		char *file = (*it1).first;
		HASH_MAP_TOKENS freqTable = (*it1).second;

		//cout << &file[strlen(cwd)] << ":" << endl;
		cout << file << ":" << endl;
		parser.mapIterate(freqTable);
	}
}

HASH_MAP_OCCURENCE_TABLE VectorFactory::createCorpusOccurenceTable
(
		HASH_MAP_PARSED_DOCS parsedDocs
) 
{
	int result;
	HASH_MAP_OCCURENCE_TABLE occurenceTable;

	HASH_MAP_PARSED_DOCS::const_iterator it1;
	for ( it1=parsedDocs.begin() ; it1 != parsedDocs.end(); it1++ )
	{
		char *file = (*it1).first;
		HASH_MAP_TOKENS freqTable = (*it1).second;

		HASH_MAP_TOKENS::const_iterator it2;
		for ( it2=freqTable.begin() ; it2 != freqTable.end(); it2++)
		{
			char *token = (*it2).first;
			HASH_MAP_TOKENS::const_iterator it3;
			it3 = freqTable.find(token);
			result = (it3 != freqTable.end()?1:0);

			if (result == 1) {
				occurenceTable[token] += 1;
			}
			else {
				occurenceTable[token] = 1;
			}
		}
	}

	return(occurenceTable);
}


