#ifndef DEFINITIONS_H
#define DEFINITIONS_H


#include <string>
#include <vector>
#include <thread>

#define DEFAULT_USERS 50
#define DEFAULT_BALANCE 2500

#define SEARCHER_QUEUE_NAME "/searcherQueue"
#define SEARCHER_RESPONSE_QUEUE_NAME "/searcherResponseQueue"

#define PAYMENT_QUEUE_NAME "/paymentQueue"
#define PAYMENT_RESPONSE_QUEUE_NAME "/paymentResponseQueue"

#define FREE_USER_SEARCH_LIMIT 100
#define NUM_THREADS_FREE_SEARCH 4



struct rechargeRequest_t {
    int user_id;
    
};

enum class UserType {
    FREE,
    PREMIUM,
    PREMIUM_UNLIMITED
};

struct Ocurrence {
    std::string prevWord;
    std::string nextWord;
    int lineNum;
    std::thread::id threadId;
    std::string fileName;
    

    bool operator<(const Ocurrence& other) const {
        return lineNum < other.lineNum;
    }
};

struct searchRequest_t {
    int user_id;
    UserType type;
    char keyword[50];
    int balance;
    int numThreads;
    char queue_name[50];  // Name of the user's message queue
};

struct searchResponse_t {
    int user_id;
    int ocurrences;
    int balance;
};

#endif // DEFINITIONS_H