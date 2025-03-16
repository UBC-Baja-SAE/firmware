/**
 * @brief Temporary file, sample code.
 */

#include "can_interface.h"
#include "can_message.h"

int main(int argc, char **argv)
{
    can_init();

    can_filter_init(socket_fd, 0x123, 0x7ff);

    struct can_message msg;
    can_receive(socket_fd, &msg);
}