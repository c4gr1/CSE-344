#include "common.h"
#include "protocol.h"
#include "utils.h"
#include <pthread.h>
#include <unistd.h>
#include <math.h>

/*
 * Global variables to define the town dimensions
 * Side effects:
 * - These are used by the delivery threads to generate random delivery locations.
 */
static int town_width;
static int town_height;

/*
 * Structure representing a delivery person
 * Side effects:
 * - Contains thread information and delivery status.
 */
typedef struct {
    pthread_t thread;
    int id;
    int capacity;
    int load;
    float velocity;
} DeliveryPerson;

/*
 * Constant defining the maximum capacity of deliveries each delivery person can carry
 * Side effects:
 * - Used to set the capacity for each delivery person.
 */
#define DELIVERY_CAPACITY 3

/*
 * Global variables to manage delivery personnel and synchronization
 * Side effects:
 * - Used to control delivery threads and manage order delivery.
 */
static DeliveryPerson *delivery_personnel;
static int num_delivery_personnel;
static pthread_mutex_t delivery_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t delivery_cond = PTHREAD_COND_INITIALIZER;
static int delivery_order_available = 0;
static int delivery_order_id = 0;

/*
 * Function prototypes for internal use
 */
void *delivery_thread(void *arg);
void simulate_delivery(float x, float y, float velocity);

/*
 * Initialize delivery personnel and create their threads
 * Side effects:
 * - Allocates memory for delivery personnel structures.
 * - Starts threads for each delivery person.
 */
void start_delivery_system(int num_deliveries, int delivery_speed, int width, int height) {
    printf("Initializing delivery personnel...\n");
    delivery_personnel = malloc(num_deliveries * sizeof(DeliveryPerson));
    num_delivery_personnel = num_deliveries;
    town_width = width;
    town_height = height;

    for (int i = 0; i < num_deliveries; i++) {
        delivery_personnel[i].id = i;
        delivery_personnel[i].capacity = DELIVERY_CAPACITY;
        delivery_personnel[i].load = 0;
        delivery_personnel[i].velocity = (float)delivery_speed / 60.0;
        if (pthread_create(&delivery_personnel[i].thread, NULL, delivery_thread, (void *)&delivery_personnel[i]) != 0) {
            perror("Failed to create delivery thread");
            exit(EXIT_FAILURE);
        }
    }
    printf("Delivery personnel initialized...\n");
}

/*
 * Function representing the delivery thread for each delivery person
 * Side effects:
 * - Continuously checks for available orders and processes them.
 */
void *delivery_thread(void *arg) {
    DeliveryPerson *person = (DeliveryPerson *)arg;

    while (1) {
        pthread_mutex_lock(&delivery_mutex);
        while (!delivery_order_available) {
            pthread_cond_wait(&delivery_cond, &delivery_mutex);
        }
        delivery_order_available--;
        int order_id = ++delivery_order_id;
        pthread_mutex_unlock(&delivery_mutex);

        char message[256];
        // Generate random position within the town
        float posX = (float)(rand() % (town_width + 1));
        float posY = (float)(rand() % (town_height + 1));
        snprintf(message, sizeof(message), "Order %d is delivering by deliver %d to address (%.2f, %.2f)", order_id, person->id, posX, posY);
        log_message(message);

        // Simulate delivery
        simulate_delivery(posX, posY, person->velocity);

        snprintf(message, sizeof(message), "Order %d is delivered by deliver %d to address (%.2f, %.2f)", order_id, person->id, posX, posY);
        log_message(message);

        pthread_mutex_lock(&delivery_mutex);
        person->load = 0;
        pthread_mutex_unlock(&delivery_mutex);

        printf("Delivery person %d completed delivery.\n", person->id);
    }

    return NULL;
}

/*
 * Simulate the delivery process by sleeping for the calculated time based on distance
 * Side effects:
 * - Uses sleep to simulate delivery time.
 */
void simulate_delivery(float x, float y, float velocity) {
    float distance = sqrt(x * x + y * y);
    int sleepTime = (int)(distance / velocity);
    sleep(sleepTime);
}

/*
 * Signal delivery personnel that an order is available
 * Side effects:
 * - Updates the condition variable to wake up a delivery thread.
 */
void signal_delivery_personnel() {
    pthread_mutex_lock(&delivery_mutex);
    delivery_order_available++;
    pthread_cond_signal(&delivery_cond);
    pthread_mutex_unlock(&delivery_mutex);
}
