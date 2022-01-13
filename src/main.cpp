/**
 * @file main.cpp
 * @author erim erkin dogan - 2019400225
 * @brief Main class for an abstractor, that searches given words in given files and calculates closeness score
 * 
 *  This program utilizes multiple threads to read and score word availability in given abstracts. Searched words,
 *  thread count and returned result size is all depends on given input file. Outputs are printed to the file given in
 *  command line execution, also the program will delete the contents of the file before writing to it. Input file will
 *  be read from main process and correspoding threads and list will be created. The reading is done via line by line and 
 *  word by word (ignoring whitespaces). The program also compares number of abstract files and number of threads and if the
 *  number of abstract files given is smaller than thread count, it will change the count to abstract file count.
 * 
 *  There are 4 global variables that will be shared across all threads: a string vector holding the words that will be searched (words);
 *  a queue that stores unprocessed abstracts(waitingAbstracts); vector of shared pointers to Abstract objects that stores score, 
 *  summary and filename (processedAbstracts); an output file stream so that all threads can write what they are processing (outputFile).
 *  Operations on these 4 global variables are done with 2 mutex locks in different parts of the function: queueMutex is locked when a 
 *  thread tries to find a new abstract to read and process, and write the information that it is processing that abstract; vectorPushMutex
 *  is used for pushing created shared pointers to Abstract objects so it is locked for that operation only. Also use of shared_ptr ensures
 *  that created object is accessible from main thread which then sorts and parses it to generate results and outputs.
 * 
 *  Note: While reading abstracts the path is set in a way that all the abstracts will be in ../abstracts folder relative to the executable.
 *  So if the program is ran from another directory which doesn't have the abstacts folder that stores the files, the output will be wrong.
 * 
 *  Input arguments needs to be given in ABSOLUTE PATH.
 *  Running:
 *  > make
 *  > ./algorithm.out ABSOLUTE_PATH_OF_INPUT_FILE     ABSOLUTE_PATH_OF_OUTPUT_FILE
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <queue>
#include <memory>
#include <algorithm>
#include <pthread.h>

#include "abstract.h"

// MUTEX INITIALIZATION
pthread_mutex_t vectorPushMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;

// GLOBAL VARIABLES
vector<string> targetWords;
queue<string> waitingAbstracts;
vector<shared_ptr<Abstract>> processedAbstracts;

// OUTPUT FILE STREAM
ofstream outputFile;

/**
 * @brief multithread part of the program, gets location and name of abstract file from queue and calculates the score, then pushes result to a vector
 * 
 * @param params character of thread (like thread A, B, C)
 */
void *calculateScore(void *params)
{
    while (1)
    {
        // Initialize strings and vectors to store read data
        string token;
        vector<vector<string>> tokenizedData;
        vector<string> sentence;

        // ID of thread
        char id = *(char *)params;

        ///////////////////////
        // Critical path, lock mutex for queue and printing to file
        pthread_mutex_lock(&queueMutex);

        // Check if any element is left in the queue, if queue is empty quit
        if (waitingAbstracts.empty())
        {
            pthread_mutex_unlock(&queueMutex);
            break;
        }

        // Get the first element from queue and remove it
        string poppedAbstract = waitingAbstracts.front();
        waitingAbstracts.pop();

        outputFile << "Thread " << id << " is calculating " << poppedAbstract << "\n";

        pthread_mutex_unlock(&queueMutex); // Unlock mutex

        ///////////////////////////

        // Opens input abstract file
        ifstream abstractFile;
        string absolutePath = "../abstracts/" + poppedAbstract;
        abstractFile.open(absolutePath);

        // Reading word by word seperated by whitespace
        while (abstractFile >> token)
        {
            sentence.push_back(token);

            // If the read character is a dot, finish the sentence and push it.
            if (token.compare(".") == 0)
            {
                tokenizedData.push_back(sentence);
                sentence.clear();
            }
        }

        abstractFile.close(); // Close the file

        // Sets to keep unique words
        unordered_set<string> intersectionOfWords;
        unordered_set<string> unionOfWords;

        // Creating a shared pointer to a new Abstract object which will store our results
        shared_ptr<Abstract> currentAbstract = make_shared<Abstract>(poppedAbstract);

        // Iterating by word by word basis on sentences
        for (auto tokenizedSentence : tokenizedData)
        {
            bool sentenceHasTheWord = false;
            for (auto tokenizedItem : tokenizedSentence)
            {
                // Compare the abstract content with given words via word by word comparison
                for (auto word : targetWords)
                {
                    // Adding given words to the unique words set
                    unionOfWords.insert(word);

                    // Adding word to the union
                    unionOfWords.insert(tokenizedItem);

                    // Comparing words, and if the sentence has the word, add it to the summary.
                    if (word.compare(tokenizedItem) == 0)
                    {
                        // If a sentence has more than 1 intersect with words, this ensures sentence is not added again
                        if (!sentenceHasTheWord)
                        {
                            currentAbstract->summary.push_back(tokenizedSentence);
                            sentenceHasTheWord = true;
                        }
                        intersectionOfWords.insert(tokenizedItem);
                    }
                }
            }
        }

        // Assigning tokens and score to class
        currentAbstract->tokens = tokenizedData;
        currentAbstract->score = 1.0 * intersectionOfWords.size() / unionOfWords.size();

        //////////////////////////////////////////////
        // Locking the mutex to push object to vector
        pthread_mutex_lock(&vectorPushMutex);
        processedAbstracts.push_back(currentAbstract);
        pthread_mutex_unlock(&vectorPushMutex);
        //////////////////////////////////////////////
    }

    pthread_exit(NULL);
}

// Comparison function for Abstract class according to their scores
bool compareAbstracts(shared_ptr<Abstract> a1, shared_ptr<Abstract> a2)
{
    if (abs(a1->score - a2->score) < 0.0001)
    {
        return false;
    }
    else
    {
        return a1->score > a2->score;
    }
}

int main(int argc, char *argv[])
{
    int threadCount, abstractCount, returnCount;
    ifstream inputFile;
    stringstream str_stream;
    string line;

    // Checking input args
    if (argc < 3)
    {
        cout << "[ERROR] Usage " << argv[0] << "input_file_location output_file_location\n";
        return -1;
    }

    // Setting args to file name
    string inputFileName = argv[1];
    string outputFileName = argv[2];

    // Opening file
    inputFile.open(inputFileName);

    // Reading header line to decipher data
    getline(inputFile, line);
    stringstream headerStream(line);
    headerStream >> threadCount >> abstractCount >> returnCount;

    // Reading reference words
    string word;
    getline(inputFile, line);
    stringstream word_stream(line);

    // Tokenizes target words and pushes to the vector
    while (word_stream >> word)
    {
        targetWords.push_back(word);
    }

    // Reading soon to be processed abstracts
    while (getline(inputFile, line))
    {
        waitingAbstracts.push(line);
    }

    // Checks if thread count is lower than file count. If it is lower, sets thread count equal to file count
    if (threadCount > abstractCount)
    {
        threadCount = abstractCount;
    }

    // Opens the outputfile for threads to use
    outputFile.open(outputFileName, ofstream::trunc);

    // Creates threads
    pthread_t threads[threadCount];
    char thread_names[threadCount];
    for (int i = 0; i < threadCount; i++)
    {
        // Gives each thread a name and passes the name as parameter
        thread_names[i] = 'A' + i;
        pthread_create(&threads[i], NULL, calculateScore, &thread_names[i]);
    }

    // Wait for threads to finish
    for (auto thr : threads)
    {
        pthread_join(thr, NULL);
    }

    // Print as threads finish running
    outputFile << "###\n";

    // Sort the vector according to its score in descending order
    sort(processedAbstracts.begin(), processedAbstracts.end(), compareAbstracts);

    // Prints the number of results to the file, sorted in descending order
    for (int i = 0; i < returnCount; i++)
    {

        Abstract item = *processedAbstracts[i];

        // Sets precision to 4 and fixes length
        outputFile.precision(4);
        outputFile << fixed;

        // Prints to file
        outputFile << "Result " << i + 1 << ":\nFile: " << item.filename << "\nScore: " << item.score << "\nSummary: ";

        // Prints summary
        for (auto sentence : item.summary)
        {
            for (auto word : sentence)
            {
                outputFile << word << " ";
            }
        }
        outputFile << "\n###\n";
    }

    // Closes the file
    outputFile.close();

    return 0;
}