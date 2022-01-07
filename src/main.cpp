#include "abstract.h"

#include <iomanip>
#include <iostream> 
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <queue>
#include <algorithm> 
#include <pthread.h>
#include <math.h>

pthread_mutex_t processed_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t unprocessed_mutex = PTHREAD_MUTEX_INITIALIZER;
vector<string> words;
queue<Abstract *> unprocessed_abstracts;
vector<Abstract *> processed_abstracts;
ofstream outputFile;

void *calculateScore(void *params)
{
    while (1)
    {
        Abstract *current_abstract;
        char id = *(char *)params;

        // Critical path, lock mutex for queue and printing to file
        pthread_mutex_lock(&unprocessed_mutex);

        // Check if any element is left in the queue, if queue is empty quit
        if (unprocessed_abstracts.empty()){
            pthread_mutex_unlock(&unprocessed_mutex);
            break;
        }

        // Get the first element from queue and remove it
        current_abstract = unprocessed_abstracts.front();
        unprocessed_abstracts.pop();

        outputFile << "Thread " << id << " is calculating " << current_abstract->filename << "\n";

        pthread_mutex_unlock(&unprocessed_mutex);   // Unlock mutex



        // Initialize strings and vectors to store read data
        string token;
        vector<vector<string>> tokenized_data;
        vector<string> sentence;

        // Opens input abstract file
        ifstream abstractFile;
        string testName = "../abstracts/" + current_abstract->filename;
        abstractFile.open(testName);

        // Reading word by word seperated by whitespace
        while (abstractFile >> token)
        {
            sentence.push_back(token);

            if (token.compare(".") == 0)
            {
                tokenized_data.push_back(sentence);
                sentence.clear();
            }
        }

        abstractFile.close();   // Close the file

        // Set to keep unique words
        unordered_set<string> interception_of_words;
        unordered_set<string> union_of_words;


        for (auto word : words)
        {
            union_of_words.insert(word);

            for (auto sentence : tokenized_data)
            {

                for (auto token : sentence)
                {
                    union_of_words.insert(token);
                    if (word.compare(token) == 0)
                    {
                        current_abstract->summary.push_back(sentence);
                        interception_of_words.insert(token);
                    }
                }
            }
        }
        current_abstract->tokens = tokenized_data;
        current_abstract->score = 1.0 * interception_of_words.size() / union_of_words.size();

        pthread_mutex_lock(&processed_mutex);
        processed_abstracts.push_back(current_abstract);
        pthread_mutex_unlock(&processed_mutex);
    }

    pthread_exit(NULL);
}

bool compareAbstract(Abstract *a1, Abstract *a2)
{
    return a1->score > a2->score;
}

int main(int argc, char *argv[])
{
    int threadCount, abstractCount, returnCount;

    if (argc < 3)
    {
        cout << "[ERROR] Usage " << argv[0] << "input_file_location output_file_location\n";
        return -1;
    }
    string inputFileName = argv[1];
    string outputFileName = argv[2];

    ifstream inputFile;

    inputFile.open(inputFileName);

    string line;

    // Reading header line to decipher data
    getline(inputFile, line);
    stringstream header_stream(line);
    header_stream >> threadCount >> abstractCount >> returnCount;

    // Reading reference words
    string word;
    getline(inputFile, line);
    stringstream word_stream(line);
    while (word_stream >> word)
    {
        words.push_back(word);
    }

    // Reading abstracts
    while (getline(inputFile, line))
    {
        unprocessed_abstracts.push(new Abstract(line));
    }

    pthread_t threads[threadCount];

    outputFile.open(outputFileName);

    if (threadCount > abstractCount) {
        threadCount = abstractCount;
    }

    for (int i = 0; i < threadCount; i++)
    {
        char *id = new char('A' + i);
        pthread_create(&threads[i], NULL, calculateScore, id);
    }

    for (auto thr : threads)
    {
        pthread_join(thr, NULL);
    }

    sort(processed_abstracts.begin(), processed_abstracts.end(), compareAbstract);
    outputFile << "###\n";
    for (int i = 0; i < returnCount; i++)
    {
        Abstract *item = processed_abstracts[i];
        outputFile.precision(4);
        outputFile << fixed;
        outputFile << "Result " << i+1 << ":\nFile: " << item->filename << "\nScore: " << item->score << "\nSummary: ";

        for (auto sentence : item->summary)
        {
            for (auto word : sentence)
            {
                outputFile << word << " ";
            }
        }
            outputFile << "\n###\n";
    }

    outputFile.close();

    return 0;
}