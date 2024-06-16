#include <iostream>
#include <thread>
#include <unistd.h>
//#include "SistemaDePago.h"
#include "Searcher.h"
#include "utils.h"
#include "User.h"
#include "definitions.h"
#include <cstring> // Include the necessary header file for the strcpy function
#include <csignal> // Include the necessary header file for the signal function
#include <mqueue.h> // Include the necessary header file for the mq_open function
#include <fcntl.h> // Include the necessary header file for the O_CREAT flag
#include <random>
#include <chrono>
#include <vector>
// Include for shm functions
#include <sys/mman.h>
#include <sys/stat.h>
#include "SistemaDePago.h"
#include <fstream>
#include <semaphore.h>
#include <iostream> // Include the necessary header file

std::vector<std::string> message_queues;
std::mutex message_queues_mutex;
int numUsers = 2;

void user_running(int numThreads, std::string keyword, User user, mqd_t searcher_mq) {

    std::cout << "[USER " << user.getId() << "] Starting to work..." << std::endl;

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

    // Print the searchRequest data (all fields) sent
    std::cout << "[USER " << user.getId() << "] Search Request Data:\n"
          << "User ID: " << searchRequest.user_id << "\n"
          << "User Type: " << UserTypeString << "\n"
          << "Keyword: " << searchRequest.keyword << "\n"
          << "Balance: " << searchRequest.balance << "\n"
          << "Number of Threads: " << searchRequest.numThreads << "\n"
          << "Queue Name: " << searchRequest.queue_name << std::endl;

    mq_send(searcher_mq, (char *)&searchRequest, sizeof(searchRequest), 0);
    std::cout << "[USER " << user.getId() << "] Request sent to the searcher for user " << user.getId() << std::endl;

    char response[2048];
    ssize_t bytes_read;
    bytes_read = mq_receive(user_mq, response, sizeof(response), NULL);
    if(bytes_read == -1) {
        perror("mq_receive_user_running_user_mq");
        return;
    }
    // Print received bytes
    std::cout << "[USER " << user.getId() << "] Bytes read AAA: " << bytes_read << std::endl;
    std::cout << "[USER " << user.getId() << "] Searcher response received for user " << user.getId() << std::endl;
    // Print the response message from the searcher (the number of ocurrences of the keyword in the dictionary)
    searchResponse_t* searchResponse = reinterpret_cast<searchResponse_t*>(response);
    //Print all the response
    std::cout << "[USER " << user.getId() << "] Searcher response: " << searchResponse->ocurrences << std::endl;
    std::cout << "[USER " << user.getId() << "] Searcher response: " << searchResponse->user_id << std::endl;
    std::cout << "[USER " << user.getId() << "] Searcher response: " << searchResponse->balance << std::endl;

    user.setBalance(searchResponse->balance);
}

int user_generator(unsigned int n_users) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 5);
    std::uniform_int_distribution<> disBalance(20, 30); // Para saldo entre 20 y 50
    std::uniform_int_distribution<> disType(0, 2); // Para tipo de usuario entre 0 y 2
    std::uniform_int_distribution<> disThreads(1, 5); // Para tipo de usuario entre 0 y 2
    std::vector<std::thread> userThreads;
    mqd_t searcher_mq;
    mqd_t searcher_response_mq;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = 128;
    attr.mq_curmsgs = 0;

    searcher_mq = mq_open(SEARCHER_QUEUE_NAME, O_CREAT | O_WRONLY, 0644, NULL);
    if(searcher_mq == -1) {
        perror("mq_open_user_generator");
        return -1;
    }

    std::vector<std::string> dictionary;
    std::string word;
    std::ifstream file("./assets/dictionary.txt"); // Reemplaza "dictionary.txt" con la ruta a tu archivo

    while (file >> word) {
        dictionary.push_back(word);
    }

    std::uniform_int_distribution<> disDictionary(0, dictionary.size() - 1); // Distribución para índices de diccionario

    for (int i = 0; i < n_users; ++i) {
        int tiempoEspera = dis(gen);
        int balance = disBalance(gen);
        int type = disType(gen);
        int numThreads = disThreads(gen);
        UserType userType;


        switch(1) {
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
        // Print the user information with printf function
        printf("User %d created with balance %d and type %d\n", i, balance, type);
        //Print de keyword from dictionary chosen
        std::cout << "Keyword: " << keyword << std::endl;
        
        userThreads.push_back(std::thread(user_running, numThreads, keyword, user, searcher_mq));

    }


    for (auto& th : userThreads) {  
        th.join();
    }

}

/*Function that encapsulate the process of creating and running the payment process. The objetive is delete the code in main function that manages the payment system and put it here*/
int running_payment_process(mqd_t payment_request_mq, mqd_t payment_response_mq) {
    /*Setting the payments msg queues*/
    /*Create semaphore that makes the system payment waits until the search tell him he read the response*/
    //Print que el running payment process esta funcionando ya
    std::cout << "[PAYMENT SYSTEM] Payment process started" << std::endl;
    sem_t* payment_sem = sem_open("/semaphore_payment", O_CREAT, 0644, 0);
    if (payment_sem == SEM_FAILED) {
        std::cerr << "Error al crear el semáforo: " << strerror(errno) << std::endl;
        return -1;
    }

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
        // Loop waiting requests for the queue Payment to process them
        while (true) {
            bytes_read_payment = mq_receive(payment_request_mq, (char *)&paymentBuffer, 128, NULL);
            // Create a pointer to the payment request struct
            rechargeRequest_t* payRequest = reinterpret_cast<rechargeRequest_t*>(paymentBuffer);
            //Print the payRequest info
            std::cout << "[PAYMENT SYSTEM] User ID: " << payRequest->user_id << std::endl;
            std::string semaphoreName = "/semaphore_search_" + std::to_string(payRequest->user_id);
            sem_t* sem = sem_open(semaphoreName.c_str(), O_CREAT, 0644, 1);
            // Print that the payment system has received a payment request from the searcher for the user id

            if (bytes_read_payment > 0) {
                // Print that the payment system has received a payment request from the searcher for the user id
                std::cout << "[PAYMENT] Payment request received from the searcher for user " << payRequest->user_id << std::endl;
                int payment = sistemaDePago->procesarPago();
                // Print the payment success message with the user id
                printf("Payment processed for user %d\n", payRequest->user_id);
                // Send the payment confirmation to the Searcher (the confirmation is just to send int 1
                mq_send(payment_response_mq, (char *)&payment, sizeof(int), 0);
                // Signal to the semaphore that the payment has been processed
                sem_post(sem);

            }

            sem_wait(payment_sem);
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
        printf("[SEARCHER] Searcher process started\n");
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
    std::cout << "Caught signal " << sig << std::endl;
    mq_unlink(SEARCHER_QUEUE_NAME);
    mq_unlink(PAYMENT_QUEUE_NAME);
    mq_unlink(PAYMENT_RESPONSE_QUEUE_NAME);
    mq_unlink(SEARCHER_RESPONSE_QUEUE_NAME);
    
    std::lock_guard<std::mutex> lock(message_queues_mutex);
    for (const auto& queue_name : message_queues) {
        mq_unlink(queue_name.c_str());
        printf("Queue %s unlinked\n", queue_name.c_str());
    }

    // Unlink the semaphores
    sem_unlink("/semaphore_payment");
    for (int i = 0; i < numUsers; i++) {
        std::string semaphoreName = "/semaphore_search_" + std::to_string(i);
        sem_unlink(semaphoreName.c_str());
        printf("Semaphore %s unlinked\n", semaphoreName.c_str());
    }

    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    mqd_t payment_request_mq;
    mqd_t payment_response_mq;

    struct mq_attr payment_attr;
    payment_attr.mq_flags = 0;
    payment_attr.mq_maxmsg = 10;
    payment_attr.mq_msgsize = 128;
    payment_attr.mq_curmsgs = 0;

    payment_request_mq = mq_open(PAYMENT_QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &payment_attr);
    if(payment_request_mq == -1) {
        perror("mq_open_payment_request_main");
        return -1;
    }

    payment_response_mq = mq_open(PAYMENT_RESPONSE_QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &payment_attr);
    if(payment_response_mq == -1) {
        perror("mq_open_payment_response_main");
        return -1;
    }

    running_search_process();
    running_payment_process(payment_request_mq, payment_response_mq);
    // Hacer que el proceso aqui se duerma durante 5 segundos
    std::this_thread::sleep_for(std::chrono::seconds(10));
    user_generator(numUsers);
    
    while (true) {};

    std::cout << "Simulación completa. Todos los usuarios han terminado sus actividades." << std::endl;
    
    return 0;
}