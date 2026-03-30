#include "../Inc/nrf24_handler.h"
#include "../Inc/data_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <thread>
#include <atomic>
#include <chrono>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#endif

// ─── NRF24L01 Registers ───────────────────────────────────────────────────────

#define NRF_REG_CONFIG      0x00
#define NRF_REG_EN_AA       0x01   // Auto-acknowledgement
#define NRF_REG_EN_RXADDR   0x02
#define NRF_REG_SETUP_AW    0x03   // Address width
#define NRF_REG_SETUP_RETR  0x04   // Retransmit config
#define NRF_REG_RF_CH       0x05   // RF channel
#define NRF_REG_RF_SETUP    0x06   // RF power / data rate
#define NRF_REG_STATUS      0x07
#define NRF_REG_RX_ADDR_P0  0x0A
#define NRF_REG_TX_ADDR     0x10
#define NRF_REG_RX_PW_P0   0x11   // Payload width pipe 0
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD       0x1C
#define NRF_REG_FEATURE     0x1D

// Commands
#define NRF_CMD_R_REGISTER  0x00
#define NRF_CMD_W_REGISTER  0x20
#define NRF_CMD_R_RX_PAYLOAD 0x61
#define NRF_CMD_W_TX_PAYLOAD 0xA0
#define NRF_CMD_FLUSH_TX    0xE1
#define NRF_CMD_FLUSH_RX    0xE2
#define NRF_CMD_NOP         0xFF

// Status flags
#define NRF_STATUS_RX_DR    0x40   // RX data ready
#define NRF_STATUS_TX_DS    0x20   // TX data sent
#define NRF_STATUS_MAX_RT   0x10   // Max retransmits reached

// Config
#define NRF_PAYLOAD_SIZE    32
#define NRF_CHANNEL         76     // 2.476 GHz — away from WiFi/BT congestion
#define NRF_SPI_DEVICE      "/dev/spidev0.0"
#define NRF_SPI_SPEED_HZ    8000000  // 8 MHz

// 5-byte pipe address — must match car-side firmware
static const uint8_t PIPE_ADDRESS[5] = { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };

// ─── State ────────────────────────────────────────────────────────────────────

static int           spi_fd = -1;
static std::atomic<bool> nrf_running(false);
static std::thread   nrf_thread;

// ─── SPI Helpers ─────────────────────────────────────────────────────────────

#ifdef __linux__

static bool spiTransfer(const uint8_t* tx, uint8_t* rx, size_t len) {
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
    return rx[0]; // status byte
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

// ─── Init ─────────────────────────────────────────────────────────────────────

static bool nrfInit() {
    // Open SPI device
    spi_fd = open(NRF_SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("NRF24: Failed to open SPI device");
        return false;
    }

    // SPI mode 0
    uint8_t mode = SPI_MODE_0;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("NRF24: Failed to set SPI mode");
        return false;
    }

    uint8_t bits = 8;
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);

    uint32_t speed = NRF_SPI_SPEED_HZ;
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // Allow power-on settling time
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Disable auto-ack (simple fire-and-forget from car side)
    nrfWriteRegister(NRF_REG_EN_AA, 0x00);

    // Enable pipe 0 RX
    nrfWriteRegister(NRF_REG_EN_RXADDR, 0x01);

    // 5-byte address width
    nrfWriteRegister(NRF_REG_SETUP_AW, 0x03);

    // No retransmit (AA disabled)
    nrfWriteRegister(NRF_REG_SETUP_RETR, 0x00);

    // RF channel 76 (2.476 GHz)
    nrfWriteRegister(NRF_REG_RF_CH, NRF_CHANNEL);

    // 2Mbps, max PA power (PA+LNA module)
    nrfWriteRegister(NRF_REG_RF_SETUP, 0x0E);

    // Set pipe addresses
    nrfWriteRegisterMulti(NRF_REG_RX_ADDR_P0, PIPE_ADDRESS, 5);
    nrfWriteRegisterMulti(NRF_REG_TX_ADDR,    PIPE_ADDRESS, 5);

    // Fixed 32-byte payload on pipe 0
    nrfWriteRegister(NRF_REG_RX_PW_P0, NRF_PAYLOAD_SIZE);

    // No dynamic payload
    nrfWriteRegister(NRF_REG_DYNPD,   0x00);
    nrfWriteRegister(NRF_REG_FEATURE, 0x00);

    // Clear any pending interrupts
    nrfWriteRegister(NRF_REG_STATUS,
                     NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
    nrfFlushRx();
    nrfFlushTx();

    // Power up in RX mode (PRX=1, PWR_UP=1, CRC=1 byte)
    nrfWriteRegister(NRF_REG_CONFIG, 0x0B); // 0b00001011

    // 1.5ms startup delay
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    printf("NRF24: Initialized on %s, channel %d\n", NRF_SPI_DEVICE, NRF_CHANNEL);
    return true;
}

// ─── Packet Dispatch ─────────────────────────────────────────────────────────

static void dispatchPacket(const NrfPacket& pkt) {
    auto& dm = DataManager::getInstance();

    switch (pkt.raw[0]) {
    case PKT_ECU_FL:
    case PKT_ECU_FR:
    case PKT_ECU_RL:
    case PKT_ECU_RR: {
        const NrfEcuPayload& e = pkt.ecu;
        DataManager::EcuPosition pos = static_cast<DataManager::EcuPosition>(e.packet_type - 1);

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
        // Commands are Pi→Car only, ignore if received
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
            // Read 32-byte payload
            uint8_t tx[NRF_PAYLOAD_SIZE + 1] = {};
            uint8_t rx[NRF_PAYLOAD_SIZE + 1] = {};
            tx[0] = NRF_CMD_R_RX_PAYLOAD;

            if (spiTransfer(tx, rx, NRF_PAYLOAD_SIZE + 1)) {
                NrfPacket pkt;
                memcpy(pkt.raw, &rx[1], NRF_PAYLOAD_SIZE);
                dispatchPacket(pkt);
            }

            // Clear RX_DR flag
            nrfWriteRegister(NRF_REG_STATUS, NRF_STATUS_RX_DR);
        }

        // Poll at 1kHz — fast enough for 100Hz sensor data with headroom
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
        // Power down before closing
        nrfWriteRegister(NRF_REG_CONFIG, 0x00);
        close(spi_fd);
        spi_fd = -1;
    }

    printf("NRF24: Stopped\n");
#endif
}

bool sendCommand(uint8_t command_id, const uint8_t* data, uint8_t len) {
#ifdef __linux__
    if (spi_fd < 0 || !nrf_running.load()) return false;

    // Switch to TX mode
    nrfWriteRegister(NRF_REG_CONFIG, 0x0A); // PWR_UP=1, PRIM_RX=0

    NrfPacket pkt;
    memset(pkt.raw, 0, NRF_PAYLOAD_SIZE);
    pkt.cmd.packet_type = PKT_CMD;
    pkt.cmd.command_id  = command_id;
    memcpy(pkt.cmd.payload, data, (len > 30) ? 30 : len);

    // Write payload
    uint8_t tx[NRF_PAYLOAD_SIZE + 1];
    uint8_t rx[NRF_PAYLOAD_SIZE + 1];
    tx[0] = NRF_CMD_W_TX_PAYLOAD;
    memcpy(&tx[1], pkt.raw, NRF_PAYLOAD_SIZE);
    spiTransfer(tx, rx, NRF_PAYLOAD_SIZE + 1);

    // CE pulse to trigger transmission (assumes CE tied high via PA+LNA module)
    std::this_thread::sleep_for(std::chrono::microseconds(15));

    // Wait for TX_DS or MAX_RT
    uint8_t status = 0;
    for (int i = 0; i < 100; i++) {
        status = nrfGetStatus();
        if (status & (NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT)) break;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    bool ok = (status & NRF_STATUS_TX_DS) != 0;
    nrfWriteRegister(NRF_REG_STATUS, NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
    nrfFlushTx();

    // Switch back to RX mode
    nrfWriteRegister(NRF_REG_CONFIG, 0x0B);

    return ok;
#else
    return false;
#endif
}