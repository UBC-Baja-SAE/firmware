#pragma once

#include <QObject>
#include <QSettings>
#include <QCoreApplication>

class AppSettings : public QObject {
    Q_OBJECT

    // Define QML-accessible properties
    Q_PROPERTY(bool websocketEnabled READ websocketEnabled WRITE setWebsocketEnabled NOTIFY websocketEnabledChanged)
    Q_PROPERTY(bool mcapEnabled READ mcapEnabled WRITE setMcapEnabled NOTIFY mcapEnabledChanged)

public:
    explicit AppSettings(QObject *parent = nullptr) : QObject(parent) {
        QSettings settings(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat);
        m_websocketEnabled = settings.value("Foxglove/EnableWebsocket", true).toBool();
        m_mcapEnabled = settings.value("Foxglove/EnableMcap", false).toBool();
    }

    bool websocketEnabled() const { return m_websocketEnabled; }
    void setWebsocketEnabled(bool val) {
        if (m_websocketEnabled == val) return;
        m_websocketEnabled = val;

        // Save to INI file instantly
        QSettings settings(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat);
        settings.setValue("Foxglove/EnableWebsocket", val);

        emit websocketEnabledChanged(val);
    }

    bool mcapEnabled() const { return m_mcapEnabled; }
    void setMcapEnabled(bool val) {
        if (m_mcapEnabled == val) return;
        m_mcapEnabled = val;

        QSettings settings(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat);
        settings.setValue("Foxglove/EnableMcap", val);

        emit mcapEnabledChanged(val);
    }

    signals:
        void websocketEnabledChanged(bool enabled);
    void mcapEnabledChanged(bool enabled);

private:
    bool m_websocketEnabled;
    bool m_mcapEnabled;
};