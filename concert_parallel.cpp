#include <iostream>
#include <pthread.h>
#include <queue>
#include <fstream>
#include <unistd.h> 
#include <cstdlib>  
#include <ctime>
#include <vector>

#define MAX_BUFFER_SIZE 5 // Maximum size of customerRequests queue
#define MAX_AGENTS 5      // Number of consumer threads

using namespace std;

int totalTickets, numAgents = MAX_AGENTS;
pthread_mutex_t mutexQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condQueue = PTHREAD_COND_INITIALIZER;

struct customer {
    string name;
    int tickets;
};
typedef struct customer Customer;

// Shared queue for customer requests
queue<Customer> customerRequests;
bool finishedReading = false; // Flag to indicate producer is done


void* bookTicket(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutexQueue);
        
        // Wait if queue is empty and producer is not finished
        while (customerRequests.empty() && !finishedReading) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        // If queue is empty and producer is finished then exit thread
        if (customerRequests.empty() && finishedReading) {
            pthread_mutex_unlock(&mutexQueue);
            break;
        }

        // Process a ticket request
        Customer currentCustomer = customerRequests.front();
        customerRequests.pop();

        int ticketsRequested = currentCustomer.tickets;
        string name = currentCustomer.name;

        if (totalTickets >= ticketsRequested) {
            totalTickets -= ticketsRequested;
            cout << "Customer " << name << " given " << ticketsRequested << " tickets\n";
        } 
        else if (totalTickets <= 0) {
            cout << "Customer " << name << " given 0 tickets\n";
        } 
        else {
            cout << "Customer " << name << " given " << totalTickets << " tickets\n";
            totalTickets = 0;
        }

        pthread_mutex_unlock(&mutexQueue);
    }
    return NULL;
}


// Function executed by the producer thread
void* producer(void* arg) {
    ifstream file("sample_input.txt");
    if (!file) {
        cerr << "Error: Could not open sample_input.txt\n";
        exit(1);
    }

    string name;
    file >> totalTickets; // Read total tickets available
    int tickets;

    while (file >> name >> tickets) {
        Customer customer = {name, tickets};

        
        cout << "Customer " << name << " requested " << tickets << " tickets\n";

        // Wait if queue is full
        pthread_mutex_lock(&mutexQueue);
        while (customerRequests.size() >= MAX_BUFFER_SIZE) {
            pthread_mutex_unlock(&mutexQueue);
            usleep(100000);
            pthread_mutex_lock(&mutexQueue);
        }

        // Add request to queue
        customerRequests.push(customer);
        pthread_cond_signal(&condQueue); 
        pthread_mutex_unlock(&mutexQueue);
    }

    file.close();

    // Signal that producer is finished
    pthread_mutex_lock(&mutexQueue);
    finishedReading = true;
    pthread_cond_broadcast(&condQueue);
    pthread_mutex_unlock(&mutexQueue);

    return NULL;
}

int main() {
    pthread_t producerThread;
    pthread_t agents[MAX_AGENTS];

    
    pthread_create(&producerThread, NULL, producer, NULL);

    
    for (int i = 0; i < MAX_AGENTS; i++) {
        pthread_create(&agents[i], NULL, bookTicket, NULL);
    }

    
    pthread_join(producerThread, NULL);

    
    for (int i = 0; i < MAX_AGENTS; i++) {
        pthread_join(agents[i], NULL);
    }

    cout << "Remaining tickets: " << totalTickets << endl;
    return 0;
}