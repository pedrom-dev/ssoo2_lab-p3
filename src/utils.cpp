
#include <mqueue.h>
#include <cstdlib> // Include the necessary header file for the exit function
#include <cstdio> // Include the necessary header file for the perror function
#include <sys/mman.h> // Include the necessary header file for the mmap function
#include <fcntl.h> // Include the necessary header file for the shm_open function
#include "User.h"
#include <unistd.h> // Include the necessary header file for the ftruncate function

// Function to create a shared memory User object
void create_shared_memory_user(User** user, const char *shared_memory_name) {

    int shm_fd = shm_open(shared_memory_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open_user_create");
        std::exit(1); // Use std::exit instead of exit
    }

    // Configure the size of the shared memory
    ftruncate(shm_fd, sizeof(User));

    // Map the shared memory in the address space of the process
    *user = (User*)mmap(0, sizeof(User), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (*user == MAP_FAILED) {
        perror("mmap");
        std::exit(1);
    }
}

// Function to get the shared memory User object
void get_shared_memory_user(User *user, const char *shared_memory_name) {
    // Open the shared memory object
    int shm_fd = shm_open(shared_memory_name, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open_user_get");
        std::exit(1); // Use std::exit instead of exit
    }

    // Map the shared memory in the address space of the process
    *user = *(User*)mmap(0, sizeof(User), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
}