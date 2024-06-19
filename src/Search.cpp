#include <string>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include "User.h"
#include "definitions.h"
#include "utils.h"
#include <atomic> // Include the <atomic> header
#include "Search.h"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h> // Include the <sys/mman.h> header
#include <semaphore.h>
#include "log.h"

std::atomic<int> totalOccurrences(0); // Variable global para contar las ocurrencias totales;
std::atomic<int> premiumBalance(0);
static std::mutex mtx; // Mutex para proteger la sección crítica
// Variable de semaforo POSIX llamado paymentSemaphore


Search::Search(searchRequest_t request) {

    // Get the files to search. You have to fill your vector files with the routes of the files to search that are in the file paths.txt (1 path per lane)
    std::ifstream pathsFile("assets/paths.txt");
    std::string path;
    while (std::getline(pathsFile, path)) {
        files.push_back(path);
    }
    
    this->request = request;

    
}

void Search::FreeUserSearch(int numThreads) {
    log("[SEARCHER] Free user search for user " + std::to_string(request.user_id) + " started");

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(&Search::FreeSearchInFile, this, files[i], request));
    }

    for (auto& th : threads) {
        th.join();
    }
}

void Search::FreeSearchInFile(const std::string& file, searchRequest_t request) {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string thread_id = oss.str();

    log("[SEARCHER] Thread (" + thread_id + ") for user " + std::to_string(request.user_id) + " searching in file " + file);
    std::ifstream inFile(file);
    if (!inFile) {
        std::cerr << "Error al abrir el archivo: " << file << std::endl;
        return;
    }
    std::string line;
    int lineNum = 0;

    while (std::getline(inFile, line)) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::istringstream iss(line);
        std::string prevWord, word, nextWord;
        bool isFirstWord = true;  // Para manejar el primer caso donde no hay palabra anterior
        while (iss >> word) {
            if (!isFirstWord && word == std::string(request.keyword)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                log("[SEARCHER (Free search)] Ocurrence found (" + std::to_string(totalOccurrences.load()) + ") for user " + std::to_string(request.user_id));
                if (iss >> nextWord) {
                    std::lock_guard<std::mutex> lock(mtx);
                    if (totalOccurrences >= FREE_USER_SEARCH_LIMIT) {
                        log("[SEARCHER] Free user search limit reached for user " + std::to_string(request.user_id));
                        return; // Si se ha alcanzado el límite, termina la ejecución del hilo
                    }
                    ++totalOccurrences;
                    // Registrar la ocurrencia
                    Ocurrence ocurrence = {prevWord, nextWord, lineNum, std::this_thread::get_id(), file};
                    ocurrencias.push_back(ocurrence);
                }
            }
            prevWord = word;
            isFirstWord = false;  // Después de la primera palabra, actualizar la bandera
        }
        ++lineNum;
    }
}


int Search::getTotalOccurrences() {
    return totalOccurrences.load();
}

void Search::unlimitedUserSearch(int numThreads) {
    std::vector<std::thread> threads;
    log("[SEARCHER] Unlimited user search started");
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(&Search::FreeSearchInFile, this, files[i], request));
    }

    for (auto& th : threads) {
        th.join();
    }
    
    for (const auto& file : files) {
        UnlimitedSearchInFile(file, keyword);
    }
}

void Search::UnlimitedSearchInFile(const std::string& file, const std::string& keyword) {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string thread_id = oss.str();

    log("[SEARCHER] Thread (" + thread_id + ") searching in file " + file);
    std::ifstream inFile(file);
    if (!inFile) {
        std::cerr << "Error al abrir el archivo: " << file << std::endl;
        return;
    }
    std::string line;
    int lineNum = 0;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string prevWord, word, nextWord;
        bool isFirstWord = true;  // Para manejar el primer caso donde no hay palabra anterior
        while (iss >> word) {
            if (!isFirstWord && word == std::string(request.keyword)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (iss >> nextWord) {
                    std::lock_guard<std::mutex> lock(mtx);
                    log("[SEARCHER (Premium Unlimited search)] Ocurrence found (" + std::to_string(totalOccurrences.load()) + ") for user " + std::to_string(request.user_id));
                    ++totalOccurrences;
                    // Registrar la ocurrencia
                    Ocurrence ocurrence = {prevWord, nextWord, lineNum, std::this_thread::get_id(), file};
                    ocurrencias.push_back(ocurrence);
                }
            }
            prevWord = word;
            isFirstWord = false;  // Después de la primera palabra, actualizar la bandera
        }
        ++lineNum;
    }
}


void Search::PremiumSearchInFile(const std::string& file, mqd_t paymentQueue, mqd_t paymentQueueResponse) {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string thread_id = oss.str();

    log("[SEARCHER] Thread (" + thread_id + ") searching in file " + file);
    std::ifstream inFile(file);
    std::string line;
    int lineNum = 0;

    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string prevWord, word, nextWord;
        bool isFirstWord = true;  // Para manejar el primer caso donde no hay palabra anterior

        while (iss >> word) {
            if (!isFirstWord && word == std::string(request.keyword)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::lock_guard<std::mutex> lock(mtx);
                if (iss >> nextWord) {
                   log("[SEARCHER (Premium search)] Ocurrence found (" + std::to_string(totalOccurrences.load()) + ") for user " + std::to_string(request.user_id));
                    if (premiumBalance <= 0) {
                        log("[SEARCHER (Premium search)] Premium user (" + std::to_string(request.user_id) + ") balance is 0");
                        // Send a rechargeRequest message to the paymentQueue
                        rechargeRequest_t rechargeRequest;
                        rechargeRequest.user_id = request.user_id;
                        mq_send(paymentQueue, (char*)&rechargeRequest, sizeof(rechargeRequest), 0);
                        log("[SEARCHER (Premium search)] Recharge request sent to paymentQueue for user " + std::to_string(request.user_id));
                        char bufferRece[128];
                        mq_receive(paymentQueueResponse, bufferRece, sizeof(bufferRece), NULL);
                        log("[SEARCHER (Premium search)] Recharge response received from paymentQueue for user for the amount of " + std::to_string(*reinterpret_cast<int*>(bufferRece)));
                        // Receiving a int from the paymentQueue2, printing the message with the rechargeAmount, adding it to the variable premiumBalance and increasing the totalOccurrences
                        int rechargeAmount = *reinterpret_cast<int*>(bufferRece);
                        premiumBalance += rechargeAmount;
                    }
                    premiumBalance--;  // Decrease the balance
                    ++totalOccurrences;
                    Ocurrence ocurrence = {prevWord, nextWord, lineNum, std::this_thread::get_id(), file};
                    ocurrencias.push_back(ocurrence);
                }
            }
            prevWord = word;
            isFirstWord = false; 
        }
        ++lineNum;
    }
}


void Search::PremiumUserSearch(int numThreads) {
    log("[SEARCHER] Premium user search started for user " + std::to_string(request.user_id));

    struct mq_attr payment_attr;
    payment_attr.mq_flags = 0;
    payment_attr.mq_maxmsg = 10;
    payment_attr.mq_msgsize = 128;
    payment_attr.mq_curmsgs = 0;

    mqd_t paymentQueue = mq_open(PAYMENT_QUEUE_NAME, O_WRONLY | O_CREAT, 0644, &payment_attr);
    if (paymentQueue == -1) {
        perror("mq_open_paymentQueue");
        return;
    }

    std::string queueName = "/search_payment_response_" + std::to_string(request.user_id);
    mqd_t paymentQueueResponse = mq_open(queueName.c_str(), O_RDWR | O_CREAT, 0644, &payment_attr);
    if (paymentQueueResponse == -1) {
        perror(("mq_open_" + queueName).c_str());
        return;
    }   


    premiumBalance = request.balance;
    std::vector<std::thread> threads;

   for (int i = 0; i < numThreads; ++i) {
        // Pasar el semáforo a cada hilo
        threads.push_back(std::thread(&Search::PremiumSearchInFile, this, files[i], paymentQueue, paymentQueueResponse));
    }
    for (auto& th : threads) {
        th.join();
    }


}

void Search::GenerateSearchResultsFile(int user_id, const std::vector<Ocurrence>& ocurrences, const char* keyword) {
    std::string userDir = "user_directory/" + std::to_string(user_id);
    std::string fileName = userDir + "/search_results.txt";
    FILE* file = fopen(fileName.c_str(), "w");
    if (file == NULL) {
        perror("fopen");
        return;
    }
    for (const auto& ocurrence : ocurrences) {
        fprintf(file, "[%d :: %s] :: %d :: %s %s %s\n", ocurrence.threadId, ocurrence.fileName.c_str(), ocurrence.lineNum, ocurrence.prevWord.c_str(), std::string(keyword).c_str(), ocurrence.nextWord.c_str());
    }
    fclose(file);
}