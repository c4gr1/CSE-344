#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_AUTOMOBILE 8
#define MAX_PICKUP 4

// Shared parking space counters
int mFreeTemp_automobile = MAX_AUTOMOBILE; // Number of available temporary parking spots for automobiles
int mFreeTemp_pickup = MAX_PICKUP; // Number of available temporary parking spots for pickups
int mFreePerm_automobile = MAX_AUTOMOBILE; // Number of available permanent parking spots for automobiles
int mFreePerm_pickup = MAX_PICKUP; // Number of available permanent parking spots for pickups

// Semaphores
sem_t newPickup, newAutomobile; // Semaphores to signal new vehicle arrivals
sem_t inChargeforPickup, inChargeforAutomobile; // Semaphores to indicate valets are available
pthread_mutex_t lockTempAutomobile, lockTempPickup; // Mutexes for temporary parking
pthread_mutex_t lockPermAutomobile, lockPermPickup; // Mutexes for permanent parking

void* carOwner(void* arg); // Function representing car owners
void* carAttendant(void* arg); // Function representing valets
void initialize(); // Initialize semaphores and mutexes
void destroy(); // Destroy semaphores and mutexes
int randomVehicleType(); // Generate a random vehicle type

int running = 1; // Global flag to stop threads

int main() {
    initialize(); // Initialize all semaphores and mutexes

    // Create carAttendant threads (one for each type)
    pthread_t attendantAutomobile, attendantPickup;
    pthread_create(&attendantAutomobile, NULL, carAttendant, (void*) "Automobile");
    pthread_create(&attendantPickup, NULL, carAttendant, (void*) "Pickup");

    // Create carOwner threads (simulating vehicle arrivals)
    pthread_t owners[20];
    for (int i = 0; i < 20; i++) {
        pthread_create(&owners[i], NULL, carOwner, (void*) (long) randomVehicleType());
        usleep(100000); // Simulate staggered arrivals (introduce a delay between arrivals)
    }

    // Join all owner threads
    for (int i = 0; i < 20; i++) {
        pthread_join(owners[i], NULL);
    }

    // Signal the attendants to stop running
    running = 0;
    sem_post(&newAutomobile); // Ensure the automobile thread wakes up and exits
    sem_post(&newPickup); // Ensure the pickup thread wakes up and exits

    // Join attendant threads
    pthread_join(attendantAutomobile, NULL);
    pthread_join(attendantPickup, NULL);

    destroy(); // Destroy all semaphores and mutexes
    return 0;
}

void initialize() {
    // Initialize semaphores
    sem_init(&newPickup, 0, 0);
    sem_init(&newAutomobile, 0, 0);
    sem_init(&inChargeforPickup, 0, 1);
    sem_init(&inChargeforAutomobile, 0, 1);
    // Initialize mutex locks
    pthread_mutex_init(&lockTempAutomobile, NULL);
    pthread_mutex_init(&lockTempPickup, NULL);
    pthread_mutex_init(&lockPermAutomobile, NULL);
    pthread_mutex_init(&lockPermPickup, NULL);
    // Seed for random number generation
    srand(time(NULL));
}

void destroy() {
    // Destroy semaphores
    sem_destroy(&newPickup);
    sem_destroy(&newAutomobile);
    sem_destroy(&inChargeforPickup);
    sem_destroy(&inChargeforAutomobile);
    // Destroy mutex locks
    pthread_mutex_destroy(&lockTempAutomobile);
    pthread_mutex_destroy(&lockTempPickup);
    pthread_mutex_destroy(&lockPermAutomobile);
    pthread_mutex_destroy(&lockPermPickup);
}

// Function to generate a random vehicle type (0: Pickup, 1: Automobile)
int randomVehicleType() {
    return rand() % 2; // 0: Pickup, 1: Automobile
}

void* carOwner(void* arg) {
    int vehicleType = (int)(long) arg; // Vehicle type (0: Pickup, 1: Automobile)
    int isAutomobile = vehicleType; // Is it an automobile? (0: Pickup, 1: Automobile)

    if (isAutomobile) {
        pthread_mutex_lock(&lockTempAutomobile); // Lock temporary automobile parking
        if (mFreeTemp_automobile > 0) {
            // If there is space in the temporary lot, reduce available spots and signal the valet
            mFreeTemp_automobile--;
            printf("Car Owner (Automobile) arrived at temporary lot. Available Temporary Automobile Spots: %d\n", mFreeTemp_automobile);
            pthread_mutex_unlock(&lockTempAutomobile);
            sem_post(&newAutomobile);
            sem_wait(&inChargeforAutomobile); // Wait for the valet to park the automobile
        } else {
            pthread_mutex_unlock(&lockTempAutomobile);
            printf("Car Owner (Automobile) leaves due to no temporary spots.\n");
        }
    } else {
        pthread_mutex_lock(&lockTempPickup); // Lock temporary pickup parking
        if (mFreeTemp_pickup > 0) {
            // If there is space in the temporary lot, reduce available spots and signal the valet
            mFreeTemp_pickup--;
            printf("Car Owner (Pickup) arrived at temporary lot. Available Temporary Pickup Spots: %d\n", mFreeTemp_pickup);
            pthread_mutex_unlock(&lockTempPickup);
            sem_post(&newPickup);
            sem_wait(&inChargeforPickup); // Wait for the valet to park the pickup
        } else {
            pthread_mutex_unlock(&lockTempPickup);
            printf("Car Owner (Pickup) leaves due to no temporary spots.\n");
        }
    }

    return NULL;
}

void* carAttendant(void* arg) {
    char* type = (char*) arg; // Valet type (Automobile or Pickup)
    int isAutomobile = (strcmp(type, "Automobile") == 0); // Does the valet handle automobiles?

    while (running) {
        if (isAutomobile) {
            sem_wait(&newAutomobile); // Wait for a new automobile to arrive
            if (!running) break;
            pthread_mutex_lock(&lockPermAutomobile); // Lock permanent automobile parking
            if (mFreePerm_automobile > 0) {
                // If there is space in the permanent lot, reduce available spots and increase temporary parking
                mFreePerm_automobile--;
                pthread_mutex_lock(&lockTempAutomobile);
                mFreeTemp_automobile++;
                printf("Valet parks an Automobile in the permanent lot. Available Permanent Automobile Spots: %d, Temporary Automobile Spots: %d\n",
                       mFreePerm_automobile, mFreeTemp_automobile);
                pthread_mutex_unlock(&lockTempAutomobile);
            } else {
                printf("No permanent spots available for Automobile.\n");
            }
            pthread_mutex_unlock(&lockPermAutomobile);
            sem_post(&inChargeforAutomobile); // Signal that the automobile has been parked
        } else {
            sem_wait(&newPickup); // Wait for a new pickup to arrive
            if (!running) break;
            pthread_mutex_lock(&lockPermPickup); // Lock permanent pickup parking
            if (mFreePerm_pickup > 0) {
                // If there is space in the permanent lot, reduce available spots and increase temporary parking
                mFreePerm_pickup--;
                pthread_mutex_lock(&lockTempPickup);
                mFreeTemp_pickup++;
                printf("Valet parks a Pickup in the permanent lot. Available Permanent Pickup Spots: %d, Temporary Pickup Spots: %d\n",
                       mFreePerm_pickup, mFreeTemp_pickup);
                pthread_mutex_unlock(&lockTempPickup);
            } else {
                printf("No permanent spots available for Pickup.\n");
            }
            pthread_mutex_unlock(&lockPermPickup);
            sem_post(&inChargeforPickup); // Signal that the pickup has been parked
        }
    }

    return NULL;
}
