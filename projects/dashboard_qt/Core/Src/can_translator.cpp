#include "../Inc/can_translator.h"
#include "../Inc/data_manager.h"

void processIncomingCanFrame(int can_id, uint8_t* data) {
    if (can_id == 0x200) {
        uint32_t rpm = (data[0] << 8) | data[1];

        DataManager::getInstance().setTach(rpm);
    }
}