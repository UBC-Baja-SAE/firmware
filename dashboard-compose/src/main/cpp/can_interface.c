#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <net/if.h>

#include "can_message.h"

/**
 * @brief The name of the CAN interface being used. This should match the name
 * of the CAN network interface on the device.
 * @see `docs/dashboard-compose.md`.
 */
#define can_interface "can0"

/**
 * @brief Returns the number of bytes of the `can_frame` struct declared in
 * `linux/can.h`. This is required to determine how many bytes to read or write
 * with CAN.
 */
#define can_frame_size() (sizeof(struct can_frame))

/**
 * @brief A bit mask for the allowable bit range for the CAN id and mask. For
 * standard CAN with an 11-bit id, this can be used to truncate larger values.
 */
#define can_id_bits 0x7ff

int socket_fd;

struct sockaddr_can addr;

struct ifreq ifr;

struct can_filter filter[1];

int can_send(int fd, struct can_message msg)
{
    struct can_frame frame;
    frame.can_id = msg.id & can_id_bits;
    frame.can_dlc = msg.size;
    // convert from proprietary CAN message to proper CAN frame.
    memcpy(frame.data, msg.data, msg.size);

    if (write(fd, &frame, can_frame_size()) != can_frame_size()) {
        printf("error sending CAN message\n");
        return 0;
    }
    return 1;
}

int can_receive(int fd, struct can_message* msg)
{
    struct can_frame frame;
    if (read(fd, &frame, can_frame_size()) < 0) {
        printf("error receiving CAN message\n");
        return 0;
    }
    memcpy(msg, frame.data, frame.can_dlc);
    printf("0x%03X [%d] ", frame.can_id, frame.can_dlc);
    for (int i = 0; i < frame.can_dlc; i++)
    {
		printf("%02X ", frame.data[i]);
    }
    printf("\n");

    return 1;
}

int can_init()
{
    printf("starting CAN bus interface\n");

    if ((socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        printf("error initialising CAN socket\n");
		return 0;
	}

    strcpy(ifr.ifr_name, can_interface);
    ioctl(socket_fd, SIOCGIFINDEX, &ifr);

    memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(socket_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        printf("error binding CAN interface\n");
        return 0;
	}

    return 1;
}

void can_filter_init(int fd, int id, int mask)
{
    filter[0].can_id = id & can_id_bits;
    filter[0].can_mask = mask & can_id_bits;

    setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));
}