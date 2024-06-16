#ifndef SEARCHER_H
#define SEARCHER_H

#include "definitions.h"
#include <queue>
#include <mutex>
#include <mqueue.h>
#include "utils.h"
#include "Search.h"
#include <unistd.h>

class Searcher {
protected:
    static Searcher* instance;
    static std::mutex bufferMutex;
    std::queue<searchRequest_t> PremiumRequestBuffer;
    std::queue<searchRequest_t> FreeRequestBuffer;
    
    bool running;
    int premiumCount;
    int freeCount;
    const int premiumWeight;
    const int freeWeight;

    Searcher();
    ~Searcher();

public:
    Searcher(const Searcher&) = delete;
    Searcher& operator=(const Searcher&) = delete;

    static Searcher* getInstance();
    void receiveRequests();
    void processRequests();
    
};

#endif // SEARCHER_H