#include <string>
#include <math.h>
#include <hash_map.h>

#define HASH_MAP_DOCUMENT_VECTOR hash_map<char*, float, hash<char*>, eqstr>

using namespace std;

class DocumentVector {
    HASH_MAP_DOCUMENT_VECTOR elements;

    public:
    void addElement(char *token, float value) ;
    int containsToken(char *token) ;
    float getTokenWeight(char *token) ;
    int dimensionality();
    float magnitude();
    DocumentVector* normalize();
    float getSimilarity(DocumentVector *dv2);
    float dotProduct(DocumentVector *dv2);
    string toString();    
};

void DocumentVector::addElement(char *token, float value) 
{
    elements[token] = value;
}
    
int DocumentVector::containsToken(char *token) 
{
    int result;
    HASH_MAP_DOCUMENT_VECTOR::const_iterator it;
    it = elements.find(token);
    result = (it != elements.end()?1:0);        
    return(result);
}
        
int DocumentVector::dimensionality() {
    return(elements.size());
}
    
float DocumentVector::magnitude() {
    float sum = 0.0;
    HASH_MAP_DOCUMENT_VECTOR::const_iterator it;
    
    for (it=elements.begin(); it != elements.end(); it++)
    {
        float weight = (float)((*it).second);
        sum += weight * weight;
    }
    float magnitude = (float) sqrt(sum);
    
    return (magnitude);
}
    
DocumentVector* DocumentVector::normalize() {
    float mag = magnitude();
    
    DocumentVector *ndv = new DocumentVector();
    HASH_MAP_DOCUMENT_VECTOR::const_iterator it2;

    for (it2=elements.begin(); it2 != elements.end(); it2++)
    {
        char *token = (*it2).first;
        float weight = float((*it2).second); 
        
        ndv->addElement(token, weight/mag);
    }
    return (ndv);
}

string DocumentVector:: toString() {
    string sb("");
    sb += "[DocumentVector: {";
    
    HASH_MAP_DOCUMENT_VECTOR::const_iterator it1;

    for (it1=elements.begin(); it1 != elements.end(); it1++)
    {
        char *token = (*it1).first;
        float value = float((*it1).second); 
        char floatValue[20] = {0};
        sprintf(floatValue, "%f", value);
        string flt(floatValue);
        
        sb += "(";
        sb += token;
        sb += ", ";
        sb += flt;
        sb += ")";
        sb += ", ";
    }
    int len = sb.length();
    sb[len-2]=']';
    sb[len-1]='\0';
    return sb;
}
    
float DocumentVector::getSimilarity(DocumentVector *dv2) {
    float sim = 0.0f;
    sim = dotProduct(dv2);
    return(sim);
}
    
float DocumentVector::dotProduct(DocumentVector *dv2) {
    float dotProduct = 0.0;
    int result;
    
    if (dimensionality() > 0 && dv2->dimensionality() > 0) {
        float numerator = 0.0;
        HASH_MAP_DOCUMENT_VECTOR::const_iterator it;

        for (it=elements.begin(); it != elements.end(); it++)
        {
            char *token = (*it).first;

            HASH_MAP_DOCUMENT_VECTOR::const_iterator it2;
            it2 = dv2->elements.find(token);
            result = (it2 != dv2->elements.end()?1:0);        
            
            if (result == 1) {
                float weight1 = (*it).second;
                float weight2 = (*it2).second; 
                numerator += weight1 * weight2;
            }
        }
        dotProduct = numerator;
    }

    return (dotProduct);
}


