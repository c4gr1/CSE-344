#ifndef PROTOCOL_H
#define PROTOCOL_H

// Message types that define the operation or command
#define MSG_ORDER_REQUEST 1
#define MSG_ORDER_UPDATE 2
#define MSG_ORDER_STATUS 3
#define MSG_ERROR 4

// Order status codes
#define ORDER_ACCEPTED 100
#define ORDER_IN_PROGRESS 101
#define ORDER_COMPLETED 102
#define ORDER_READY_FOR_DELIVERY 103
#define ORDER_DELIVERED 104
#define ORDER_CANCELLED 105
#define ORDER_FAILED 106

// Error codes
#define ERR_CONNECTION_FAILED 200
#define ERR_INVALID_ORDER 201
#define ERR_SERVER_SHUTDOWN 202

// Order structure
typedef struct {
    int order_id;
    float x;          // Customer location x-coordinate
    float y;          // Customer location y-coordinate
    char details[256]; // Details of the pide order
    int status;       // Status of the order, based on protocol definitions
} Order;

#endif // PROTOCOL_H
