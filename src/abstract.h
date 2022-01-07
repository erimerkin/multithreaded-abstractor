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