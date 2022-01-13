/**
 * @file main.cpp
 * @author erim erkin dogan
 * @brief Main class for an abstractor, that searches given words in given files and calculates closeness score
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
pthread_mutex_t processed_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t unprocessed_mutex = PTHREAD_MUTEX_INITIALIZER;

// GLOBAL VARIABLES
vector<string> words;
queue<shared_ptr<Abstract>> unprocessed_abstracts;
vector<shared_ptr<Abstract>> processed_abstracts;

// OUTPUT FILE STREAM
ofstream outputFile;

void *calculateScore(void *params)
{
    while (1)
    {
        // Initialize strings and vectors to store read data
        string token;
        vector<vector<string>> tokenized_data;
        vector<string> sentence;

        // A pointer to abstract class,
        shared_ptr<Abstract> current_abstract;
        char id = *(char *)params;

        ///////////////////////
        // Critical path, lock mutex for queue and printing to file
        pthread_mutex_lock(&unprocessed_mutex);

        // Check if any element is left in the queue, if queue is empty quit
        if (unprocessed_abstracts.empty())
        {
            pthread_mutex_unlock(&unprocessed_mutex);
            break;
        }

        // Get the first element from queue and remove it
        current_abstract = unprocessed_abstracts.front();
        unprocessed_abstracts.pop();

        outputFile << "Thread " << id << " is calculating " << current_abstract->filename << "\n";

        pthread_mutex_unlock(&unprocessed_mutex); // Unlock mutex

        ///////////////////////////

        // Opens input abstract file
        ifstream abstractFile;
        string testName = "../abstracts/" + current_abstract->filename;
        abstractFile.open(testName);

        // Reading word by word seperated by whitespace
        while (abstractFile >> token)
        {
            sentence.push_back(token);

            // If the read character is a dot, finish the sentence and push it.
            if (token.compare(".") == 0)
            {
                tokenized_data.push_back(sentence);
                sentence.clear();
            }
        }

        abstractFile.close(); // Close the file

        // Sets to keep unique words
        unordered_set<string> intersection_of_words;
        unordered_set<string> union_of_words;

        // Iterating by word by word basis on sentences
        for (auto tokenized_sentence : tokenized_data)
        {
            bool found = false;
            for (auto tokenized_item : tokenized_sentence)
            {
                // Compare the abstract content with given words via word by word comparison
                for (auto word : words)
                {
                    // Adding given words to the unique words set
                    union_of_words.insert(word);

                    // Adding word to the union
                    union_of_words.insert(tokenized_item);

                    // Comparing words, and if the sentence has the word, add it to the summary.
                    if (word.compare(tokenized_item) == 0)
                    {
                        if (!found)
                        {
                            current_abstract->summary.push_back(tokenized_sentence);
                            found = true;
                        }
                        intersection_of_words.insert(tokenized_item);
                    }
                }
            }
        }

        // Assigning tokens and score to class
        current_abstract->tokens = tokenized_data;
        current_abstract->score = 1.0 * intersection_of_words.size() / union_of_words.size();

        //////////////////////////////////////////////
        // Locking the mutex to push object to vector
        pthread_mutex_lock(&processed_mutex);
        processed_abstracts.push_back(current_abstract);
        pthread_mutex_unlock(&processed_mutex);
        //////////////////////////////////////////////
    }

    pthread_exit(NULL);
}

// Comparison function for Abstract class according to their scores
bool compareAbstract(shared_ptr<Abstract> a1, shared_ptr<Abstract> a2)
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

    // Tokenizes words
    while (word_stream >> word)
    {
        words.push_back(word);
    }

    // Reading abstracts
    while (getline(inputFile, line))
    {
        unprocessed_abstracts.push(make_shared<Abstract>(line));
    }

    // Checks if thread count is lower than file count. If it is lower, sets thread count equal to file count
    if (threadCount > abstractCount)
    {
        threadCount = abstractCount;
    }

    // Opens the outputfile for threads to use
    outputFile.open(outputFileName);

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
    sort(processed_abstracts.begin(), processed_abstracts.end(), compareAbstract);

    // Prints the number of results to the file, sorted in descending order
    for (int i = 0; i < returnCount; i++)
    {

        Abstract item = *processed_abstracts[i];

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