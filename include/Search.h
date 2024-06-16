#ifndef SEARCH_H
#define SEARCH_H

#include "definitions.h"
#include <queue>
#include <mutex>
#include <mqueue.h>
#include "User.h"
#include <semaphore.h>

class Search {
   
public:

    char keyword[15];
    searchRequest_t request;
    std::vector<std::string> files;
    std::mutex mtx;
    sem_t sem;
    static mqd_t paymentQueue; // Declaración de la cola de mensajes de pago
    static mqd_t paymentQueueResponse; // Declaración de la cola de mensajes de pago
    std::vector<Ocurrence> ocurrencias;
    sem_t* paymentSemaphore;
    
    Search();
    Search(searchRequest_t request);
    void FreeUserSearch(int numThreads);
    void FreeSearchInFile(const std::string& file, searchRequest_t request);
    int getTotalOccurrences();
    void unlimitedUserSearch(int numThreads);
    void GenerateSearchResultsFile(int user_id, const std::vector<Ocurrence> &ocurrences, const char* keyword);
    void UnlimitedSearchInFile(const std::string &file, const std::string &keyword);
    void PremiumUserSearch(int numThreads);
    void PremiumSearchInFile(const std::string& file, sem_t* sem, mqd_t paymentQueue, mqd_t paymentQueueResponse);
    
};


#endif // SEARCH_H