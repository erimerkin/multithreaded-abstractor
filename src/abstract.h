/**
 * @file abstract.h
 * @author erim erkin dogan
 * @brief A class for storing info for given and read abstract files
 *  
 *  This file contains class Abstract which stores the filename, score, summary and tokenized version of the file's content.
 */

#ifndef ABSTRACT_H
#define ABSTRACT_H

#include <string>
#include <vector>

using namespace std;

class Abstract
{
public:
    double score;
    string filename;
    vector< vector<string> > tokens;
    vector< vector<string> > summary;

    Abstract(string filename);
};

#endif