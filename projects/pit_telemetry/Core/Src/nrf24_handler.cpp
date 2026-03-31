#include "../Inc/nrf24_handler.h"
#include "../Inc/data_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <gpiod.h>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#endif

// ─── NRF24L01 Registers ───────────────────────────────────────────────────────

#define NRF_REG_CONFIG      0x00
#define NRF_REG_EN_AA       0x01
#define NRF_REG_EN_RXADDR   0x02
#define NRF_REG_SETUP_AW    0x03
#define NRF_REG_SETUP_RETR  0x04
#define NRF_REG_RF_CH       0x05
#define NRF_REG_RF_SETUP    0x0E  // ← was 0x20 (wrong — that's NRF_CMD_W_REGISTER)
#define NRF_REG_STATUS      0x07
#define NRF_REG_RX_ADDR_P0  0x0A
#define NRF_REG_TX_ADDR     0x10
#define NRF_REG_RX_PW_P0    0x11
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD       0x1C
#define NRF_REG_FEATURE     0x1D

// Commands
#define NRF_CMD_R_REGISTER   0x00
#define NRF_CMD_W_REGISTER   0x20
#define NRF_CMD_R_RX_PAYLOAD 0x61
#define NRF_CMD_W_TX_PAYLOAD 0xA0
#define NRF_CMD_FLUSH_TX     0xE1
#define NRF_CMD_FLUSH_RX     0xE2
#define NRF_CMD_NOP          0xFF

// Status flags
#define NRF_STATUS_RX_DR    0x40
#define NRF_STATUS_TX_DS    0x20
#define NRF_STATUS_MAX_RT   0x10

#define NRF_PAYLOAD_SIZE  32
#define NRF_CHANNEL       76
#define NRF_SPI_DEVICE    "/dev/spidev0.0"
#define NRF_SPI_SPEED_HZ  8000000

static const uint8_t PIPE_ADDRESS[5] = { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };

// ─── State ────────────────────────────────────────────────────────────────────

static int               spi_fd = -1;
static std::mutex        spi_mutex;
static std::atomic<bool> nrf_running(false);
static std::thread       nrf_thread;

// ─── SPI Helpers ─────────────────────────────────────────────────────────────

#ifdef __linux__

static bool spiTransfer(const uint8_t* tx, uint8_t* rx, size_t len) {
    std::lock_guard<std::mutex> lock(spi_mutex);
    struct spi_ioc_transfer transfer = {};
    transfer.tx_buf        = (unsigned long)tx;
    transfer.rx_buf        = (unsigned long)rx;
    transfer.len           = len;
    transfer.speed_hz      = NRF_SPI_SPEED_HZ;
    transfer.bits_per_word = 8;
    transfer.delay_usecs   = 0;
    return ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer) >= 0;
}

static uint8_t nrfWriteRegister(uint8_t reg, uint8_t val) {
    uint8_t tx[2] = { (uint8_t)(NRF_CMD_W_REGISTER | (reg & 0x1F)), val };
    uint8_t rx[2] = {};
    spiTransfer(tx, rx, 2);
    return rx[0];
}

static uint8_t nrfReadRegister(uint8_t reg) {
    uint8_t tx[2] = { (uint8_t)(NRF_CMD_R_REGISTER | (reg & 0x1F)), NRF_CMD_NOP };
    uint8_t rx[2] = {};
    spiTransfer(tx, rx, 2);
    return rx[1];
}

static void nrfWriteRegisterMulti(uint8_t reg, const uint8_t* data, size_t len) {
    uint8_t tx[len + 1];
    uint8_t rx[len + 1];
    tx[0] = NRF_CMD_W_REGISTER | (reg & 0x1F);
    memcpy(&tx[1], data, len);
    spiTransfer(tx, rx, len + 1);
}

static uint8_t nrfGetStatus() {
    uint8_t tx = NRF_CMD_NOP;
    uint8_t rx = 0;
    spiTransfer(&tx, &rx, 1);
    return rx;
}

static void nrfFlushRx() {
    uint8_t cmd = NRF_CMD_FLUSH_RX;
    uint8_t rx  = 0;
    spiTransfer(&cmd, &rx, 1);
}

static void nrfFlushTx() {
    uint8_t cmd = NRF_CMD_FLUSH_TX;
    uint8_t rx  = 0;
    spiTransfer(&cmd, &rx, 1);
}

// ─── CE GPIO (libgpiod v2) ────────────────────────────────────────────────────

#define NRF_CE_GPIO_CHIP "/dev/gpiochip0"
#define NRF_CE_GPIO_LINE  22  // ← update to pit Pi CE GPIO

static struct gpiod_chip*         ce_chip    = nullptr;
static struct gpiod_line_request* ce_request = nullptr;

static bool ceInit() {
    ce_chip = gpiod_chip_open(NRF_CE_GPIO_CHIP);
    if (!ce_chip) {
        perror("NRF24: Failed to open GPIO chip");
        return false;
    }

    struct gpiod_line_settings* settings = gpiod_line_settings_new();
    if (!settings) return false;
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    struct gpiod_line_config* line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        gpiod_line_settings_free(settings);
        return false;
    }
    unsigned int offset = NRF_CE_GPIO_LINE;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config* req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        return false;
    }
    gpiod_request_config_set_consumer(req_cfg, "nrf24_ce_pit");

    ce_request = gpiod_chip_request_lines(ce_chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (!ce_request) {
        perror("NRF24: Failed to request CE GPIO");
        gpiod_chip_close(ce_chip);
        ce_chip = nullptr;
        return false;
    }

    return true;
}

static void ceHigh() {
    if (ce_request)
        gpiod_line_request_set_value(ce_request, NRF_CE_GPIO_LINE,
                                     GPIOD_LINE_VALUE_ACTIVE);
}

static void ceLow() {
    if (ce_request)
        gpiod_line_request_set_value(ce_request, NRF_CE_GPIO_LINE,
                                     GPIOD_LINE_VALUE_INACTIVE);
}

static void ceCleanup() {
    if (ce_request) { gpiod_line_request_release(ce_request); ce_request = nullptr; }
    if (ce_chip)    { gpiod_chip_close(ce_chip);              ce_chip    = nullptr; }
}

// ─── Init ─────────────────────────────────────────────────────────────────────

static bool nrfInit() {
    if (!ceInit()) return false;
    ceLow();

    spi_fd = open(NRF_SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("NRF24: Failed to open SPI device");
        ceCleanup();
        return false;
    }

    uint8_t mode = SPI_MODE_0;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("NRF24: Failed to set SPI mode");
        close(spi_fd); spi_fd = -1;
        ceCleanup();
        return false;
    }

    uint8_t bits = 8;
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);

    uint32_t speed = NRF_SPI_SPEED_HZ;
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    nrfWriteRegister(NRF_REG_EN_AA,      0x00);
    nrfWriteRegister(NRF_REG_EN_RXADDR,  0x01);
    nrfWriteRegister(NRF_REG_SETUP_AW,   0x03);
    nrfWriteRegister(NRF_REG_SETUP_RETR, 0x00);
    nrfWriteRegister(NRF_REG_RF_CH,      NRF_CHANNEL);
    nrfWriteRegister(NRF_REG_RF_SETUP,   0x0E);
    nrfWriteRegisterMulti(NRF_REG_RX_ADDR_P0, PIPE_ADDRESS, 5);
    nrfWriteRegisterMulti(NRF_REG_TX_ADDR,    PIPE_ADDRESS, 5);
    nrfWriteRegister(NRF_REG_RX_PW_P0,  NRF_PAYLOAD_SIZE);
    nrfWriteRegister(NRF_REG_DYNPD,     0x00);
    nrfWriteRegister(NRF_REG_FEATURE,   0x00);

    nrfWriteRegister(NRF_REG_STATUS,
                     NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
    nrfFlushRx();
    nrfFlushTx();

    nrfWriteRegister(NRF_REG_CONFIG, 0x0B); // PRX, PWR_UP, CRC
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    printf("NRF24 DIAG: CONFIG=0x%02X RF_CH=0x%02X RF_SETUP=0x%02X STATUS=0x%02X\n",
           nrfReadRegister(NRF_REG_CONFIG),
           nrfReadRegister(NRF_REG_RF_CH),
           nrfReadRegister(NRF_REG_RF_SETUP),
           nrfGetStatus());
    uint8_t addr_tx[6] = { NRF_REG_RX_ADDR_P0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t addr_rx[6] = {};
    spiTransfer(addr_tx, addr_rx, 6);
    printf("NRF24 DIAG: RX_ADDR_P0=%02X:%02X:%02X:%02X:%02X\n",
           addr_rx[1], addr_rx[2], addr_rx[3], addr_rx[4], addr_rx[5]);

    ceHigh(); // enter RX mode
    printf("NRF24: Initialized on %s, channel %d, CE=GPIO%d\n",
           NRF_SPI_DEVICE, NRF_CHANNEL, NRF_CE_GPIO_LINE);
    return true;
}

// ─── Packet Dispatch ─────────────────────────────────────────────────────────

static void dispatchPacket(const NrfPacket& pkt) {
    printf("NRF24: RX packet type 0x%02X\n", pkt.raw[0]); // remove after confirmed

    auto& dm = DataManager::getInstance();

    switch (pkt.raw[0]) {
    case PKT_ECU_FL:
    case PKT_ECU_FR:
    case PKT_ECU_RL:
    case PKT_ECU_RR: {
        const NrfEcuPayload& e = pkt.ecu;
        DataManager::EcuPosition pos =
            static_cast<DataManager::EcuPosition>(e.packet_type - 1);
        dm.setEcuTravel(pos, static_cast<float>(e.travel));
        dm.setEcuStrain(pos, static_cast<float>(e.strain_l),
                             static_cast<float>(e.strain_r));
        dm.setEcuAccel(pos,  static_cast<float>(e.accel_x),
                             static_cast<float>(e.accel_y),
                             static_cast<float>(e.accel_z));
        dm.setEcuGyro(pos,   static_cast<float>(e.gyro_x),
                             static_cast<float>(e.gyro_y),
                             static_cast<float>(e.gyro_z));
        break;
    }
    case PKT_POWERTRAIN: {
        const NrfPowertrainPayload& p = pkt.powertrain;
        dm.setTach(p.tach);
        dm.setSpeed(p.speedo);
        dm.setTemp(p.temp);
        dm.setFuel(p.fuel);
        break;
    }
    case PKT_GPS: {
        const NrfGpsPayload& g = pkt.gps;
        dm.setGps(g.latitude, g.longitude, g.gps_speed,
                  static_cast<bool>(g.has_fix));
        break;
    }
    case PKT_CMD:
        printf("NRF24: Unexpected CMD packet received (ignored)\n");
        break;
    default:
        printf("NRF24: Unknown packet type 0x%02X\n", pkt.raw[0]);
        break;
    }
}

// ─── RX Thread ───────────────────────────────────────────────────────────────

static void nrfReaderThread() {
    printf("NRF24: Listening for packets...\n");

    while (nrf_running.load()) {
        uint8_t status = nrfGetStatus();

        if (status & NRF_STATUS_RX_DR) {
            uint8_t fifo_status = nrfReadRegister(NRF_REG_FIFO_STATUS);

            if (!(fifo_status & 0x01)) {
                uint8_t tx[NRF_PAYLOAD_SIZE + 1] = {};
                uint8_t rx[NRF_PAYLOAD_SIZE + 1] = {};
                tx[0] = NRF_CMD_R_RX_PAYLOAD;

                if (spiTransfer(tx, rx, NRF_PAYLOAD_SIZE + 1)) {
                    NrfPacket pkt;
                    memcpy(pkt.raw, &rx[1], NRF_PAYLOAD_SIZE);
                    dispatchPacket(pkt);
                }
            } else {
                nrfFlushRx();
            }

            nrfWriteRegister(NRF_REG_STATUS, NRF_STATUS_RX_DR);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }

    printf("NRF24: Reader thread stopped\n");
}

#endif // __linux__

// ─── Public API ───────────────────────────────────────────────────────────────

bool startNrf24() {
#ifdef __linux__
    if (nrf_running.load()) {
        fprintf(stderr, "NRF24: Already running\n");
        return false;
    }

    if (!nrfInit()) {
        fprintf(stderr, "NRF24: Initialization failed\n");
        return false;
    }

    nrf_running.store(true);
    nrf_thread = std::thread(nrfReaderThread);
    return true;
#else
    printf("NRF24: Not available on this platform\n");
    return false;
#endif
}

void stopNrf24() {
#ifdef __linux__
    if (!nrf_running.load()) return;
    nrf_running.store(false);

    if (nrf_thread.joinable())
        nrf_thread.join();

    if (spi_fd >= 0) {
        ceLow();
        nrfWriteRegister(NRF_REG_CONFIG, 0x00);
        close(spi_fd);
        spi_fd = -1;
    }

    ceCleanup();
    printf("NRF24: Stopped\n");
#endif
}

bool sendCommand(uint8_t command_id, const uint8_t* data, uint8_t len) {
    (void)command_id; (void)data; (void)len;
    fprintf(stderr, "NRF24: sendCommand called on pit Pi — not supported\n");
    return false;
}