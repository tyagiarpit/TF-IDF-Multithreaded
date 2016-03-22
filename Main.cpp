#include <stdio.h>
#include <list>
#include <map>
#include <vector>
#include <deque>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <hash_map.h>
#include <sys/time.h>


#define FALSE 0
#define TRUE 1

#include "StopWordProcessor.cpp"
#include "StemmingProcessor.cpp"
#include "Parser.cpp"
#include "DocumentVector.cpp"
#include "VectorFactory.cpp"
#include "RedBlackTree.cpp"

using namespace std;

#ifndef HASH_MAP_TOKENS
#define HASH_MAP_TOKENS hash_map<char*, int, hash<char*>, eqstr>
#endif

#ifndef HASH_MAP_VECTOR
#define HASH_MAP_VECTOR hash_map<char*, DocumentVector*, hash<char*>, eqstr>
#endif


#define TEST_DOC_PATH "resources/test-docs"
char cwd[1024];
char base_path[1024];
list<char*> docs;   


void loadTestDocs(char *oldDirPrev) {
    DIR* dir;
    struct dirent* entry;
    struct stat dir_stat;
    char *fileName = NULL;
    char *oldDir;
    int len;
    struct dirent** namelist;
    int i, max;
    
    strcat(cwd, "/");
    dir = opendir(cwd);
    if (!dir) {
        cout << "Cannot read directory "<< cwd <<endl;
        return;
    }

    max = scandir(cwd, &namelist, NULL, alphasort);
    for (i = 0; i < max; i++) {
        /* skip the "." and ".." entries, to avoid loops. */
        if (strcmp(namelist[i]->d_name, ".") == 0)
            continue;
        if (strcmp(namelist[i]->d_name, "..") == 0)
            continue;

               /* check if the given namelist[i] is a directory. */
               if (stat(namelist[i]->d_name, &dir_stat) == -1) {
                    perror("stat:");
                    continue;
                }

        if (S_ISDIR(dir_stat.st_mode)) {
            /* Change into the new directory */
            oldDir = (char*)malloc(strlen(cwd)+1);
            strcpy(oldDir, cwd);
            strcat(cwd, namelist[i]->d_name);
            if (chdir(namelist[i]->d_name) == -1) {
                cout<< "Cannot chdir into "<<namelist[i]->d_name<<endl;
                continue;
            }
            /* check this directory */
            loadTestDocs(oldDir);

            memset(cwd, '\0', 1024);
            strcpy(cwd, oldDir);
            if (chdir("..") == -1) {
                cout << "Cannot chdir back to "<<cwd<<endl;
                exit(1);
            }           
        }
        else
        {
            /*Not a directory. check if the file ends with .txt*/
            len = strlen(namelist[i]->d_name);
            if((namelist[i]->d_name[len-4] == '.')
               && (namelist[i]->d_name[len-3] == 't')
               && (namelist[i]->d_name[len-2] == 'x')
               && (namelist[i]->d_name[len-1] == 't'))
            {
	      fileName = (char*)malloc(strlen(cwd) - strlen(oldDirPrev) + strlen(namelist[i]->d_name)+1);
	      strcpy(fileName, &cwd[strlen(oldDirPrev)]);
	      strcat(fileName, namelist[i]->d_name);
               docs.push_back(fileName);
            }
        }
    }
    free(namelist);
}

/*
 * Changed below method to accept 'number of threads' as a parameter
 */

HASH_MAP_VECTOR createVectors(list<char*> docs, int numberOfThreads) {
    StopwordProcessor stopwordProcessor;
    StemmingProcessor stemmingProcessor;
    VectorFactory v1;

    Parser parser(stopwordProcessor, stemmingProcessor);

    /*
     * Changed below method call to pass 'number of threads' as a parameter
     */
    HASH_MAP_VECTOR vectors = v1.createVectors(parser, docs, numberOfThreads);
    return(vectors);
}

void dumpVectors(HASH_MAP_VECTOR vectors) {
    HASH_MAP_VECTOR::const_iterator it1;
    
    for ( it1=vectors.begin() ; it1 != vectors.end(); it1++)
    {
        char *file1 = (*it1).first;
        DocumentVector *dv1 = (*it1).second;
        
        cout << file1 <<endl;
	//        cout << dv1->toString()<<endl;
    }
}

void dumpTop10Similarities(HASH_MAP_VECTOR vectors) {
    const float NEG_INF = -9999.0;
    float ITEM_NOT_FOUND = NEG_INF;
    HASH_MAP_VECTOR::const_iterator it1;
    list <char*> *filelst;
    list <char*> *filelist;

    for ( it1=vectors.begin() ; it1 != vectors.end(); it1++)
    {
        char *file1 = (*it1).first;
        DocumentVector *dv1 = (*it1).second;
        
//        cout<<"*** " <<&file1[strlen(cwd)]<<endl;
        cout<<"*** " <<file1<<endl;
        
        RedBlackTree *tm = new RedBlackTree( ITEM_NOT_FOUND );
        
        HASH_MAP_VECTOR::const_iterator it2;
        for ( it2=vectors.begin() ; it2 != vectors.end(); it2++)
        {
            char *file2 = (*it2).first;
            DocumentVector *dv2 = (*it2).second;
            
            float similarity = 1.0f - dv1->getSimilarity(dv2);
            if ((filelst = tm->get(similarity)) == NULL)
            {
                filelist = new list<char*>;
                filelist->push_back(file2);
                tm->insert( similarity, filelist);
            }
            else
            {
                filelst->push_back(file2);
            }
        }

        int count = 0;        
        tm->createList();
        while((tm->hasNext())&& count < 10)
        {
            RedBlackNode *node = tm->next();
            float similarity = node->getSimilarity();
            list<char*> *files=node->getFiles();
            list<char*>::iterator it = files->begin();
            
            while( (it != files->end()) && count < 10)
            {
//	      cout<<"      - " <<(1.0f - similarity) <<" = " <<&(*it)[strlen(cwd)]<<endl;
  	        cout<<"      - " <<(1.0f - similarity) <<" = " <<(*it)<<endl;
                it++;
                count++; 
            }           
        }
    }
}

struct timeval load_start;
struct timeval load_end;
struct timeval createvector_start;
struct timeval createvector_end;
struct timeval dump_start;
struct timeval dump_end;

int main(int argc, char *argv[]) {

	/*Main now to accept 1 more additional arg(number of threads)*/
    if (argc != 3)
    {
        cout << "usage ./Main base_path_with_trailing_slash number_of_threads"<<endl;
        exit(1);
    }


    memset(cwd, '\0', 1024);
    memset(base_path, '\0', 1024);
    strcpy(cwd, argv[1]);
    strcpy(base_path, argv[1]);
    strcat(cwd, TEST_DOC_PATH);
    int numberOfThreads = atoi(argv[2]);
    if (chdir(cwd) == -1) {
        cout<< "Cannot chdir to "<<cwd<<endl;
        exit(1);
    }           

    gettimeofday(&load_start, NULL); 
    loadTestDocs(NULL);
    gettimeofday(&load_end, NULL);
    long loadtime = calcDiffTime(&load_start, &load_end);
    printf("loadtime = %ld\n", loadtime);
    
    gettimeofday(&createvector_start, NULL); 

    /*updated method call to pass threads as a paremeter*/
    HASH_MAP_VECTOR vectors = createVectors(docs, numberOfThreads);
    gettimeofday(&createvector_end, NULL); 
    long cvtime = calcDiffTime(&createvector_start, &createvector_end);
    printf("cvtime = %ld\n", cvtime);

#ifndef AFFIXES_ONLY
    gettimeofday(&dump_start, NULL); 
    dumpTop10Similarities(vectors);
    gettimeofday(&dump_end, NULL); 
    long dumptime = calcDiffTime(&dump_start, &dump_end);
    printf("dumptime = %ld\n", dumptime);
#endif

    memset(cwd, '\0', 1024);
    memset(base_path, '\0', 1024);
    strcpy(cwd, argv[1]);
    if (chdir(cwd) == -1) {
        cout<< "Cannot chdir to "<<cwd<<endl;
        exit(1);
    }               
}
