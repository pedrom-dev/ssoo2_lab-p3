#include "SistemaDePago.h"
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <vector>
#include <mutex>


// Inicialización de la instancia del Singleton
SistemaDePago* SistemaDePago::instance = nullptr;
std::mutex pagoMutex;

SistemaDePago* SistemaDePago::getInstance() {

    std::lock_guard<std::mutex> guard(pagoMutex);
    
    if (instance == nullptr) {
        instance = new SistemaDePago();
    }
    return instance;

}

int SistemaDePago::procesarPago() {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(20, 80);

    int payment = distr(gen);

    // Simulate payment processing time
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return payment;
}

