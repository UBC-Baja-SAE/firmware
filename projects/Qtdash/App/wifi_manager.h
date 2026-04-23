#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QThread>

class WifiManager : public QObject {
    Q_OBJECT

signals:
    void networksFound(QStringList networks);
    void routingReady();
    void connectionFailed();
    void connectionSucceeded(const QString& ssid);

public:
    static WifiManager& getInstance() {
        static WifiManager instance;
        return instance;
    }

    Q_INVOKABLE void scanNetworks() {
        if (!prepareInterface()) {
            qDebug() << "Failed to prepare Wi-Fi interface";
            return;
        }
        runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 scan");
        QTimer::singleShot(4000, this, [this]() {
            QString output = runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 scan_results");
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            QStringList networks;
            for (int i = 1; i < lines.size(); ++i) {
                QStringList columns = lines[i].split('\t');
                if (columns.size() >= 5) {
                    QString ssid = columns[4].trimmed();
                    if (!ssid.isEmpty() && !networks.contains(ssid)) {
                        networks.append(ssid);
                    }
                }
            }
            emit networksFound(networks);
        });
    }

    Q_INVOKABLE bool connectToNetwork(const QString& ssid, const QString& password) {
        if (!prepareInterface()) {
            qDebug() << "Failed to prepare Wi-Fi interface";
            emit connectionFailed();
            return false;
        }

        // Remove all existing networks first for a clean connection
        runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 remove_network all");

        QString idStr = runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 add_network");
        bool ok = false;
        int id = idStr.toInt(&ok);
        if (!ok) {
            qDebug() << "Failed to add network, wpa_cli returned:" << idStr;
            emit connectionFailed();
            return false;
        }

        runCommand(QString("wpa_cli -p /var/run/wpa_supplicant -i wlan0 set_network %1 ssid '\"%2\"'").arg(id).arg(ssid));
        runCommand(QString("wpa_cli -p /var/run/wpa_supplicant -i wlan0 set_network %1 psk '\"%2\"'").arg(id).arg(password));
        runCommand(QString("wpa_cli -p /var/run/wpa_supplicant -i wlan0 enable_network %1").arg(id));
        runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 save_config");
                runCommand("cangen can0");

        // Poll for connection then set up routing
        pollForConnection(ssid, 0);
        return true;
    }

    Q_INVOKABLE void setupRouting() {
        // Request IP via DHCP
        runCommand("udhcpc -i wlan0 -q");
        QThread::msleep(2000);

        // Remove any existing default routes via usb0
        QString routes = runCommand("ip route show");
        for (const QString& line : routes.split('\n', Qt::SkipEmptyParts)) {
            if (line.contains("default") && line.contains("usb0")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                int viaIdx = parts.indexOf("via");
                if (viaIdx != -1 && viaIdx + 1 < parts.size()) {
                    QString gw = parts[viaIdx + 1];
                    runCommand(QString("ip route del default via %1 dev usb0").arg(gw));
                    qDebug() << "Removed usb0 default route via" << gw;
                }
            }
        }

        // Find wlan0 gateway from its subnet route
        QString gateway = getWlan0Gateway();
        if (gateway.isEmpty()) {
            qDebug() << "Could not determine wlan0 gateway";
            emit connectionFailed();
            return;
        }

        // Check if wlan0 default already exists
        QString currentRoutes = runCommand("ip route show");
        if (!currentRoutes.contains("default") || !currentRoutes.contains("wlan0")) {
            runCommand(QString("ip route add default via %1 dev wlan0").arg(gateway));
            qDebug() << "Added default route via" << gateway << "on wlan0";
        }

        // Set DNS
        runCommand("echo 'nameserver 8.8.8.8' > /etc/resolv.conf");
        runCommand("echo 'nameserver 1.1.1.1' >> /etc/resolv.conf");

        qDebug() << "Routing ready. Routes:";
        qDebug() << runCommand("ip route show");
        qDebug() << "wlan0 IP:" << getWlanIP();

        emit routingReady();
    }

    Q_INVOKABLE QString getWlanIP() {
        return runCommand("ip addr show wlan0 | grep 'inet ' | awk '{print $2}' | cut -d/ -f1");
    }

    Q_INVOKABLE bool isConnected() {
        QString status = runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 status | grep wpa_state");
        return status.contains("COMPLETED");
    }

    Q_INVOKABLE QString connectedSSID() {
        QString status = runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 status");
        for (const QString& line : status.split('\n', Qt::SkipEmptyParts)) {
            if (line.startsWith("ssid=")) {
                return line.mid(5).trimmed();
            }
        }
        return QString();
    }

    Q_INVOKABLE void disconnect() {
        runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 disconnect");
        runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 remove_network all");

        // Restore usb0 as default route if needed
        QString routes = runCommand("ip route show");
        if (!routes.contains("default")) {
            runCommand("ip route add default dev usb0");
            qDebug() << "Restored usb0 as default route";
        }
        qDebug() << "Disconnected from WiFi";
    }

private:
    WifiManager() {
        // On startup, restore routing if wlan0 is already connected
        QTimer::singleShot(1000, this, [this]() {
            if (isConnected()) {
                qDebug() << "wlan0 already connected on startup, restoring routing...";
                setupRouting();
            }
        });
    }

    void pollForConnection(const QString& ssid, int attempts) {
        if (attempts > 15) {
            qDebug() << "Connection to" << ssid << "timed out";
            emit connectionFailed();
            return;
        }

        QTimer::singleShot(1000, this, [this, ssid, attempts]() {
            QString status = runCommand("wpa_cli -p /var/run/wpa_supplicant -i wlan0 status");
            if (status.contains("wpa_state=COMPLETED")) {
                qDebug() << "Connected to" << ssid;
                emit connectionSucceeded(ssid);
                setupRouting();
            } else {
                qDebug() << "Waiting for connection... attempt" << attempts + 1;
                pollForConnection(ssid, attempts + 1);
            }
        });
    }

    QString getWlan0Gateway() {
        // Try to derive gateway from wlan0's assigned IP (assume .1 is gateway)
        QString ip = getWlanIP();
        if (!ip.isEmpty()) {
            QStringList parts = ip.split('.');
            if (parts.size() == 4) {
                parts[3] = "1";
                QString gateway = parts.join('.');
                qDebug() << "Derived wlan0 gateway:" << gateway;
                return gateway;
            }
        }

        // Fallback: parse from route table
        QString routes = runCommand("ip route show dev wlan0");
        for (const QString& line : routes.split('\n', Qt::SkipEmptyParts)) {
            if (line.contains("default")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                int viaIdx = parts.indexOf("via");
                if (viaIdx != -1 && viaIdx + 1 < parts.size()) {
                    return parts[viaIdx + 1];
                }
            }
        }
        return QString();
    }

    bool prepareInterface() {
        QDir dir("/var/run/wpa_supplicant");
        if (!dir.exists()) {
            runCommand("mkdir -p /var/run/wpa_supplicant");
        }
        if (!QFile::exists("/var/run/wpa_supplicant/wlan0")) {
            qDebug() << "wpa_supplicant socket missing, starting daemon...";
            runCommand("rfkill unblock wifi");
            runCommand("ip link set wlan0 up");
            runCommand("wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf");
            QThread::msleep(500);
        }
        return QFile::exists("/var/run/wpa_supplicant/wlan0");
    }

    QString runCommand(const QString& cmd) {
        QProcess process;
        process.start("sh", QStringList() << "-c" << cmd);
        process.waitForFinished(5000);
        QString output = process.readAllStandardOutput().trimmed();
        QString err = process.readAllStandardError().trimmed();
        if (!err.isEmpty()) {
            qDebug() << "CMD:" << cmd << "| ERR:" << err;
        }
        return output;
    }
};