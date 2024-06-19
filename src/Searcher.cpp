#include "Searcher.h"
#include "definitions.h"
#include <queue>
#include <mutex>
#include <mqueue.h>
#include "utils.h"
#include "Search.h"
#include <unistd.h>
#include <thread> // Include the necessary header file for using std::this_thread
#include <chrono> // Include the necessary header file for using std::chrono
#include <fcntl.h> // Include the necessary header file for using the O_RDONLY flag
#include <cstring>
#include <iostream>
#include "log.h"

Searcher* Searcher::instance = nullptr;
std::mutex Searcher::bufferMutex;

Searcher::Searcher() : premiumWeight(4), freeWeight(1)
{
    premiumCount = 0;
    freeCount = 0;
    running = true;
}

Searcher *Searcher::getInstance()
{
    std::lock_guard<std::mutex> lock(bufferMutex);
        if (instance == nullptr) {
           instance = new Searcher();
        }
    return instance;
}

void Searcher::receiveRequests() {
    mqd_t mq;
    char buffer[1024];
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;    
    attr.mq_msgsize = 1024;  
    attr.mq_curmsgs = 0;     

    mq = mq_open(SEARCHER_QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if(mq == -1) {
        perror("mq_open_receive_request_searcher");
        return;
    }
    log("[SEARCHER-RECEIVER] Receiving service started");
    ssize_t bytes_read; 
        while (running) {
            bytes_read = mq_receive(mq, buffer, 1024, NULL);
            searchRequest_t* searchRequestR = reinterpret_cast<searchRequest_t*>(buffer);
            log("[SEARCHER-RECEIVER] Request received from user " + std::to_string(searchRequestR->user_id));
                               
            if (bytes_read > 0) {
                std::lock_guard<std::mutex> lock(bufferMutex); 
                switch (searchRequestR->type)
                {
                case UserType::PREMIUM:
                    instance->PremiumRequestBuffer.push(*searchRequestR);
                    break;

                case UserType::PREMIUM_UNLIMITED:
                    instance->PremiumRequestBuffer.push(*searchRequestR);
                    break;

                case UserType::FREE:
                    instance->FreeRequestBuffer.push(*searchRequestR);
                    break;

                default:
                    fprintf(stderr, "[SEARCHER-ERR]Tipo de usuario no válido\n");
                    break;
                }
            } else {
                perror("mq_receive_searcher_error");
                running = false;
                // Stop if there's an error
            }
        }
}

void Searcher::processRequests() {
    log("[SEARCHER-PROCESSER] Processing service started");
        while (running) {
            std::lock_guard<std::mutex> lock(bufferMutex);
            if (!PremiumRequestBuffer.empty() && (premiumCount < premiumWeight || FreeRequestBuffer.empty())) {
                // Process premium request
                searchRequest_t req = PremiumRequestBuffer.front();
                PremiumRequestBuffer.pop();
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                } else if (pid == 0) {
                    // Child process
                    // Open the queue to reply to the user
                    mqd_t user_mq = mq_open(req.queue_name, O_WRONLY | O_CREAT, 0644);
                    // Check if error
                    if (user_mq == -1) {
                        perror("mq_process_request_searcher");
                        exit(1);
                    }

                    log("[SEARCHER-PROCESSER] Processing PREMIUM request for user " + std::to_string(req.user_id));
                    // Now check if the user is PremiumUserIlimited or PremiumUser
                    if (req.type == UserType::PREMIUM) {
                        Search* search = new Search(req);

                        search->PremiumUserSearch(req.numThreads);
                        log("[SEARCHER-PROCESSER] Premium search finished for user " + std::to_string(req.user_id));
                        search->GenerateSearchResultsFile(req.user_id, search->ocurrencias, req.keyword);
                        log("[SEARCHER-PROCESSER] Search results file generated for user " + std::to_string(req.user_id));
                        // Send the response to the user with a searchResponse_t struct
                        searchResponse_t response;
                        response.user_id = req.user_id;
                        response.ocurrences = search->ocurrencias.size();
                        response.balance = req.balance;

                        mq_send(user_mq, reinterpret_cast<const char*>(&response), sizeof(searchResponse_t), 0);
                        log("[SEARCHER-PROCESSER] Response sent to user " + std::to_string(req.user_id));
                        
                        
                    } else {
                        Search* search = new Search(req);
                        search->unlimitedUserSearch(req.numThreads);
                        log("[SEARCHER-PROCESSER] Premium unlimited search finished for user " + std::to_string(req.user_id));
                        search->GenerateSearchResultsFile(req.user_id, search->ocurrencias, req.keyword);
                        log("[SEARCHER-PROCESSER] Search results file generated for user " + std::to_string(req.user_id));
                        // Send the response to the user with a searchResponse_t struct
                        searchResponse_t response;
                        response.user_id = req.user_id;
                        response.ocurrences = search->ocurrencias.size();
                        response.balance = req.balance;

                        mq_send(user_mq, reinterpret_cast<const char*>(&response), sizeof(searchResponse_t), 0);
                        log("[SEARCHER-PROCESSER] Response sent to user " + std::to_string(req.user_id));

                    }
                   
                    exit(0);
                }
                premiumCount++;
                freeCount = 0;  // Reiniciar contador de gratuitos si premium se procesa
            } else if (!FreeRequestBuffer.empty() && (freeCount < freeWeight || PremiumRequestBuffer.empty())) {
                // Process free request
                searchRequest_t req = FreeRequestBuffer.front();
                FreeRequestBuffer.pop();
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                } else if (pid == 0) {
                    // Child process

                    // Open the queue to reply to the user
                    mqd_t user_mq = mq_open(req.queue_name, O_WRONLY);
                    log("[SEARCHER-PROCESSER] Processing free request for user " + std::to_string(req.user_id));
                    Search* search = new Search(req);
                    search->FreeUserSearch(NUM_THREADS_FREE_SEARCH);
                    log("[SEARCHER-PROCESSER] Free search finished for user " + std::to_string(req.user_id));
                    search->GenerateSearchResultsFile(req.user_id, search->ocurrencias, req.keyword);
                    log("[SEARCHER-PROCESSER] Search results file generated for user " + std::to_string(req.user_id));
                    // Send the response to the user with a searchResponse_t struct
                    searchResponse_t response;
                    response.user_id = req.user_id;
                    response.ocurrences = search->ocurrencias.size();
                    response.balance = req.balance;

                    mq_send(user_mq, reinterpret_cast<const char*>(&response), sizeof(searchResponse_t), 0);
                    log("[SEARCHER-PROCESSER] Response sent to user " + std::to_string(req.user_id));

                    exit(0);
                                                                       
                }
                freeCount++;
                premiumCount = 0;  // Reiniciar contador de premium si gratuito se procesa 
            }
        }
}