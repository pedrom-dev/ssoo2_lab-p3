#include <iostream>
#include <thread>
#include <unistd.h>
//#include "SistemaDePago.h"
#include "Searcher.h"
#include "utils.h"
#include "User.h"
#include "definitions.h"
#include <cstring> 
#include <csignal> 
#include <mqueue.h>
#include <fcntl.h>
#include <random>
#include <chrono>
#include <vector>
// Include for shm functions
#include <sys/mman.h>
#include <sys/stat.h>
#include "SistemaDePago.h"
#include <fstream>
#include <semaphore.h>
#include <iostream> 
#include <stdexcept> 
#include <mutex>
#include "log.h"

std::vector<std::string> message_queues;
std::mutex message_queues_mutex;
int numUsers = 15;

void user_running(int numThreads, std::string keyword, User user, mqd_t searcher_mq) {

    log("[USER " + std::to_string(user.getId()) + "] Starting to work...");
    
    auto start = std::chrono::high_resolution_clock::now();

    // Create a message queue for this user

    //Atributes for queue 
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;
    attr.mq_curmsgs = 0;


    std::string queue_name = "/queue_" + std::to_string(user.getId());
    {
        std::lock_guard<std::mutex> lock(message_queues_mutex);
        message_queues.push_back(queue_name); // Registrar el nombre de la cola de manera segura
    }
    mqd_t user_mq = mq_open(queue_name.c_str(), O_CREAT | O_RDONLY, 0644, &attr);
    if(user_mq == -1) {
        perror("mq_open_user_running");
        return;
    }

    struct searchRequest_t searchRequest;
    searchRequest.user_id = user.getId();
    searchRequest.type = user.getType();
    strcpy(searchRequest.keyword, keyword.c_str());
    searchRequest.balance = user.getBalance();
    searchRequest.numThreads = numThreads;
    strcpy(searchRequest.queue_name, queue_name.c_str());

    std::string UserTypeString;
    switch(user.getType()) {
        case UserType::FREE:
            UserTypeString = "FREE";
            break;
        case UserType::PREMIUM:
            UserTypeString = "PREMIUM";
            break;
        case UserType::PREMIUM_UNLIMITED:
            UserTypeString = "PREMIUM_UNLIMITED";
            break;
    }

    mq_send(searcher_mq, (char *)&searchRequest, sizeof(searchRequest), 0);
    log("[USER " + std::to_string(user.getId()) + "] Request sent to the searcher for user " + std::to_string(user.getId()));

    char response[2048];
    ssize_t bytes_read;
    bytes_read = mq_receive(user_mq, response, sizeof(response), NULL);
    if(bytes_read == -1) {
        perror("mq_receive_user_running_user_mq");
        return;
    }

    searchResponse_t* searchResponse = reinterpret_cast<searchResponse_t*>(response);

    user.setBalance(searchResponse->balance);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-start;

    // Crea el directorio para el usuario si no existe
    std::string dir_path = "user_directory/" + std::to_string(user.getId());
    mkdir(dir_path.c_str(), 0777);

    // Abre el archivo en el directorio del usuario
    std::ofstream outfile;
    std::string file_path = dir_path + "/info.txt";
    outfile.open(file_path, std::ios_base::app);

    // Escribe en el archivo
    outfile << "User ID: " << user.getId() << "\n";
    outfile << "Keyword: " << keyword << "\n";  
    outfile << "Número de ocurrencias: " << searchResponse->ocurrences << "\n";
    outfile << "Balance final: " << searchResponse->balance << "\n";
    outfile << "Tiempo de inicio: " << std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count() << "\n";
    outfile << "Tiempo de fin: " << std::chrono::duration_cast<std::chrono::seconds>(end.time_since_epoch()).count() << "\n";
    outfile << "Tiempo transcurrido: " << diff.count() << " segundos\n";

    outfile.close();

}

int user_generator(unsigned int n_users) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 5);
    std::uniform_int_distribution<> disBalance(20, 30); // Para saldo entre 20 y 50
    std::uniform_int_distribution<> disType(0, 2); // Para tipo de usuario entre 0 y 2

    // En lugar de calcular el número de hilos entre 1 y 5. Haz entre 1 y lo que te devuelva hardwrare_concurrency
    unsigned int nT = std::thread::hardware_concurrency();

    std::uniform_int_distribution<> disThreads(1, nT);
    std::vector<std::thread> userThreads;
    mqd_t searcher_mq;
    mqd_t searcher_response_mq;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = 128;
    attr.mq_curmsgs = 0;

    try {
        searcher_mq = mq_open(SEARCHER_QUEUE_NAME, O_CREAT | O_WRONLY, 0644, NULL);
        if(searcher_mq == -1) {
            throw std::runtime_error("mq_open_user_generator failed");
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    }

    std::vector<std::string> dictionary;
    std::string word;
    std::ifstream file("./assets/dictionary.txt"); // Reemplaza "dictionary.txt" con la ruta a tu archivo

    while (file >> word) {
        dictionary.push_back(word);
    }

    std::uniform_int_distribution<> disDictionary(0, dictionary.size() - 1); // Distribución para índices de diccionario

    log("[USER-GENERATOR] Generating " + std::to_string(n_users) + " users with random balance, type, numThreads and keyword");
    for (int i = 0; i < n_users; ++i) {
        int tiempoEspera = dis(gen);
        int balance = disBalance(gen);
        int type = disType(gen);
        int numThreads = disThreads(gen);
        UserType userType;


        switch(type) {
            case 0:
                userType = UserType::FREE;
                break;
            case 1:
                userType = UserType::PREMIUM;
                break;
            case 2:
                userType = UserType::PREMIUM_UNLIMITED;
                break;
        }

        int dictionaryIndex = disDictionary(gen);

        std::string keyword = dictionary[dictionaryIndex];

        std::string user_directory = "./user_directory/" + std::to_string(i);
        mkdir(user_directory.c_str(), 0777);
        

        User user(i, balance, userType);
        // Print the user info (id, balance, type, numThreads, keyword). All in one line and starting with [USER-GENERATOR]
        log("[USER-GENERATOR] User ID: " + std::to_string(user.getId()) + " Balance: " + std::to_string(user.getBalance()) + " NumThreads: " + std::to_string(numThreads) + " Keyword: " + keyword);
        userThreads.push_back(std::thread(user_running, numThreads, keyword, user, searcher_mq));

    }


    for (auto& th : userThreads) {  
        th.join();
    }

}

/*Function that encapsulate the process of creating and running the payment process. The objetive is delete the code in main function that manages the payment system and put it here*/
int running_payment_process() {

    log("[PAYMENT SYSTEM] Payment process started");

    // Create a process to handle the payment requests using the static instance of the SistemaDePago class
    pid_t paymentPid = fork();

    if (paymentPid == -1) {
        std::cerr << "Failed to fork()" << std::endl;
        return -1;
    } else if (paymentPid > 0) {
        // Parent process

    } else { // Child process (payment)
        ssize_t bytes_read_payment;
        char paymentBuffer[128];
        SistemaDePago* sistemaDePago = SistemaDePago::getInstance();

        struct mq_attr payment_attr;
        payment_attr.mq_flags = 0;
        payment_attr.mq_maxmsg = 10;
        payment_attr.mq_msgsize = 128;
        payment_attr.mq_curmsgs = 0;

        mqd_t payment_request_mq;
        try {
            payment_request_mq = mq_open(PAYMENT_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &payment_attr);
            if(payment_request_mq == -1) {
                throw std::runtime_error("mq_open_payment_request_main failed");
            }
        } catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
            return -1;
        }

        // Loop waiting requests for the queue Payment to process them
        while (true) {
            bytes_read_payment = mq_receive(payment_request_mq, (char *)&paymentBuffer, 128, NULL);
            // Create a pointer to the payment request struct
            rechargeRequest_t* payRequest = reinterpret_cast<rechargeRequest_t*>(paymentBuffer);

            struct mq_attr payment_attr;
            payment_attr.mq_flags = 0;
            payment_attr.mq_maxmsg = 10;
            payment_attr.mq_msgsize = 128;
            payment_attr.mq_curmsgs = 0;

            std::string queueName = "/search_payment_response_" + std::to_string(payRequest->user_id);
            mqd_t paymentQueueResponse = mq_open(queueName.c_str(), O_WRONLY | O_CREAT, 0644, &payment_attr);
            if (paymentQueueResponse == -1) {
                perror(("mq_open_" + queueName).c_str());
               return -1;
            } 

            if (bytes_read_payment > 0) {
                // Print that the payment system has received a payment request from the searcher for the user id
                log("[PAYMENT SYSTEM] Payment request received from the searcher for user " + std::to_string(payRequest->user_id));                 
                int payment = sistemaDePago->procesarPago();
                // Print the payment success message with the user id
                log("[PAYMENT SYSTEM] Payment processed for user " + std::to_string(payRequest->user_id) + " with amount " + std::to_string(payment));
                // Send the payment confirmation to the Searcher (the confirmation is just to send int 1
                mq_send(paymentQueueResponse, (char *)&payment, sizeof(int), 0);
                // Signal to the semaphore that the payment has been processed

            }
        }
    }
}

/*Function that encapsulate the process of creating and running the searcher process. The objetive is delete the code in main function that manages the searcher system and put it here*/
int running_search_process() {
    pid_t searcherPid = fork();
    if (searcherPid == -1) {
        std::cerr << "Failed to fork()" << std::endl;
        return -1;
    } else if (searcherPid > 0) {
        
    } else { // Child process (searcher)
        Searcher* searcher = Searcher::getInstance();
        log("[SEARCHER] Searcher process started");
        std::thread searcherThread([&searcher]() { searcher->receiveRequests(); });
        // Thread with the searcher processRequest method
        std::thread processRequestThread([&searcher]() { searcher->processRequests(); });

        searcherThread.join();
        processRequestThread.join();

        exit(0);
    }

}

void handle_sigint(int sig) 
{
    log("Caught signal " + std::to_string(sig));
    mq_unlink(SEARCHER_QUEUE_NAME);
    mq_unlink(PAYMENT_QUEUE_NAME);
    mq_unlink(PAYMENT_RESPONSE_QUEUE_NAME);
    mq_unlink(SEARCHER_RESPONSE_QUEUE_NAME);
    
    std::lock_guard<std::mutex> lock(message_queues_mutex);
    for (const auto& queue_name : message_queues) {
        mq_unlink(queue_name.c_str());
        log("Queue " + queue_name + " unlinked");
    }

    // Unlink the message queues
    for (int i = 0; i < numUsers; i++) {
        std::string queueName = "/search_payment_response_" + std::to_string(i);
        mq_unlink(queueName.c_str());
        log("Message queue " + queueName + " unlinked");
    }

    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    running_search_process();
    running_payment_process();
    // Hacer que el proceso aqui se duerma durante 5 segundos
    sleep(1);
    user_generator(numUsers);
    
    return 0;
}