/**
 * @file can_interface.h
 * @brief This file provides an interface to interact with the Linux kernel on
 * the Raspberry Pi to communicate with the physical CAN bus. The methods in
 * this file can be used to initialise the CAN bus and send/receive data.
 */

#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <net/if.h>
#include <linux/can.h>

#include "can_message.h"

/**
 * @brief The file descriptor for the CAN socket. This is set by `can_init`.
 */
extern int socket_fd;

/**
 * @brief The socket address for the CAN interface.
 */
extern struct sockaddr_can addr;

/**
 * @brief The interface request structure that holds information regarding the
 * CAN interface. This holds the interface name ("can0") and its interface
 * index, which is used to access the network interface for the Linux kernel.
 */
extern struct ifreq ifr;

/**
 * @brief The CAN filter for filtering received messages. This is set by 
 * `can_filter_init`.
 */
extern struct can_filter filter[1];

/**
 * @brief This sends the given message to the CAN bus.
 * 
 * @param fd    the file descriptor for the CAN socket set by `can_init`.
 * @param msg   the message to send to the CAN bus.
 * @return 1 if the message was sent successfully, else 0. 
 */
int can_send(int fd, struct can_message msg);

/**
 * @brief This blocks the calling thread and waits for the next message on the
 * CAN bus that satisifes the CAN bus filter set by `can_filter_init`.
 *
 * Once a CAN bus message is received, it writes to the contents of the message
 * pointed to by `msg`.
 * @param fd    the file descriptor for the CAN socket set by `can_init`.
 * @param msg   the pointer to the CAN message.
 * @return  1 if a message was received successfully, else 0.
 */
int can_receive(int fd, struct can_message* msg);

/**
 * @brief Initialises the CAN socket and CAN socket address for the application.
 * This initialises `socket_fd`, `addr`, and `ifr`.
 * @return  1 if the initialisation was successful, else 0.
 */
int can_init();

/**
 * @brief Initialises the CAN filter `filter`. This filters all messages
 * received by the dashboard so that `can_receive` will return a `can_message`
 * if and only if the CAN bus contains a message with an 11-bit id `rid` such
 * that `(rid & mask) == (id & mask)`.
 * @param fd    the file descriptor for the CAN socket set by `can_init`.
 * @param id    the 11-bit id to match messages with after being bit-masked.
 * @param mask  the bit mask to filter messages by.
 * @return  1 if the initialisation was successful, else 0.
 */
int can_filter_init(int fd, int id, int mask);

#endif // CAN_INTERFACE_H
