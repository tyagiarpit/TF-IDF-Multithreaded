#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <string.h>
#include <hash_map.h>
#include "ParsingModule.h"


int Parser::readLine(FILE *fp, char **line)
{
    char buffer[10000] = {0};
    int i = 0;
    int c;
    
    while ((c=fgetc(fp)) != '\n')
    {
        if (c == EOF)
            return 0;
            
        buffer[i] = c;
        i++;
    }

    *line = (char*)malloc(strlen(buffer) + 1);
    strcpy(*line, buffer);
    return 1;
}

void Parser::parseHtmlPattern(char *text)
{
    int i;
    int result;

    char *strToRead= text;
    int strt_pointer = 0;

    while(1)
    {
        regmatch_t pmatch[1];
        result = regexec(htmlRegPattern, strToRead, 1, pmatch, 0);
        
        if((result != 0) || (pmatch[0].rm_so == -1))
            break;

        /* Replace all html tags by spaces*/
        memset(text + strt_pointer + pmatch[0].rm_so,' ', pmatch[0].rm_eo - pmatch[0].rm_so);
        
        strt_pointer += pmatch[0].rm_eo;
        strToRead = text + strt_pointer;
    }
}

void Parser::parsePunctPattern(char *text)
{
    int i;
    int result;

    char *strToRead= text;
    int strt_pointer = 0;

    while(1)
    {
        regmatch_t pmatch[1];
        result = regexec(punctRegPattern, strToRead, 1, pmatch, 0);
        
        if((result != 0) || (pmatch[0].rm_so == -1))
            break;

        /* Replace all html tags by spaces*/
        memset(text + strt_pointer + pmatch[0].rm_so,' ', pmatch[0].rm_eo - pmatch[0].rm_so);
        
        strt_pointer += pmatch[0].rm_eo;
        strToRead = text + strt_pointer;
    }
}

void Parser::parseTokenPattern(char *text, HASH_MAP_TOKENS &tokenTable)
{
    int i;
    int result;
    int isStopWord;

    char *strToRead= text;
    int strt_pointer = 0;

    while(1)
    {
        regmatch_t pmatch[1];
        result = regexec(tokenRegPattern, strToRead, 1, pmatch, 0);
        
        if((result != 0) || (pmatch[0].rm_so == -1))
            break;

        /* Replace all html tags by spaces*/
        int len = pmatch[0].rm_eo - pmatch[0].rm_so;
        char *token = (char*)malloc(len+1);
        memset(token, '\0', len + 1);
        memcpy(token, text + strt_pointer+ pmatch[0].rm_so, len);
        
        isStopWord = stopwordProcessor.isStopWord(token);
        if (isStopWord == 1)
        {
            strt_pointer += pmatch[0].rm_eo;
             strToRead = text + strt_pointer;
             continue;
        }

        string word(token);
        word = stemmingProcessor.stripAffixes(word);
        char *tok = (char*)malloc(word.length() + 1);
        strcpy(tok, word.c_str());
        
        HASH_MAP_TOKENS::const_iterator it;
        it = tokenTable.find(tok);
        result = (it != tokenTable.end()?1:0);

        if (result == 1)
        {
            tokenTable[tok] = tokenTable[tok] + 1; /*increment the count*/
        }

        else
        {
            tokenTable[tok] = 1;
        }
        strt_pointer += pmatch[0].rm_eo;
        strToRead = text + strt_pointer;
    }
}

void Parser::parseStream(char *text, HASH_MAP_TOKENS &tokenTable)
{
   parseHtmlPattern(text);
   parsePunctPattern(text);
   parseTokenPattern(text, tokenTable); 
}

void Parser::mapIterate(HASH_MAP_TOKENS tokenTable)
{
    HASH_MAP_TOKENS::const_iterator it;
    for ( it=tokenTable.begin() ; it != tokenTable.end(); it++ )
    {
        cout << (*it).first << " => " << (*it).second << endl;
    }
}

HASH_MAP_TOKENS Parser::parseFile(FILE *fp)
{
    int ret;
    char *line;
    HASH_MAP_TOKENS tokenTable;
    
    while((ret = readLine(fp, &line)) == 1)
        parseStream(line, tokenTable);

    return tokenTable;
}

HASH_MAP_TOKENS Parser::parseString(char *text)
{
    HASH_MAP_TOKENS tokenTable;
    parseStream(text, tokenTable);
    return tokenTable;
}
