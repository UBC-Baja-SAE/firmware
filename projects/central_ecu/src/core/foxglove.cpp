#include "foxglove.h"
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <cmath>

FoxgloveSink::~FoxgloveSink()
{
    m_channels.clear();
    m_imageChannels.clear();
    m_audioChannels.clear();
    m_writer.reset();
    m_server.reset();
}

void FoxgloveSink::startServer(uint16_t port)
{
    foxglove::WebSocketServerOptions opts;
    opts.port = port;
    opts.host = "0.0.0.0";

    auto res = foxglove::WebSocketServer::create(std::move(opts));
    if (res.has_value()) {
        m_server = std::make_unique<foxglove::WebSocketServer>(std::move(res.value()));
        qInfo() << "Foxglove WebSocket listening on port" << port;
    } else {
        qWarning() << "Foxglove server error:" << QString::fromStdString(foxglove::strerror(res.error()));
    }
}

const FoxgloveSink::ChannelEntry *FoxgloveSink::ensureChannel(const QString &topic, const QVariantMap &values)
{
    auto it = m_channels.constFind(topic);
    if (it != m_channels.cend())
        return it->channel ? &it.value() : nullptr;

    QJsonObject props;
    for (auto v = values.cbegin(); v != values.cend(); ++v)
        props[v.key()] = QJsonObject{{"type", "number"}};
    const QJsonObject schemaObj{{"type", "object"}, {"properties", props}};

    ChannelEntry entry;
    entry.messagestorage = QJsonDocument(schemaObj).toJson(QJsonDocument::Compact).toStdString();

    const std::string nameStr = (topic.startsWith('/') ? topic.mid(1) : topic).toStdString();
    const std::string topicStr = topic.toStdString();

    foxglove::Schema schema;
    schema.name = nameStr;
    schema.encoding = "jsonschema";
    schema.data = reinterpret_cast<const std::byte*>(entry.messagestorage.data());
    schema.data_len = entry.messagestorage.size();

    auto res = foxglove::RawChannel::create(topicStr, "json", schema);
    if (res.has_value())
        entry.channel = std::make_shared<foxglove::RawChannel>(std::move(res.value()));
    else
        qWarning() << "Channel create failed for" << topic << QString::fromStdString(foxglove::strerror(res.error()));

    auto ins = m_channels.insert(topic, std::move(entry));
    return ins->channel ? &ins.value() : nullptr;
}

void FoxgloveSink::publishSample(const QString &topic, const QVariantMap &values, quint64 ts)
{
    const ChannelEntry *entry = ensureChannel(topic, values);
    if (!entry)
        return;

    m_jsonScratch.clear();
    m_jsonScratch.append('{');
    bool first = true;
    for (auto v = values.cbegin(); v != values.cend(); ++v) {
        if (!first) m_jsonScratch.append(',');
        first = false;
        m_jsonScratch.append('"');
        m_jsonScratch.append(v.key().toUtf8());
        m_jsonScratch.append("\":");

        const double d = v.value().toDouble();
        if (std::isfinite(d))
            m_jsonScratch.append(QByteArray::number(d, 'g', 9));
        else
            m_jsonScratch.append("null");
    }
    m_jsonScratch.append('}');

    entry->channel->log(reinterpret_cast<const std::byte*>(m_jsonScratch.constData()), m_jsonScratch.size(), ts);
}

void FoxgloveSink::publishImage(const QString &cam, const QByteArray &jpeg, quint64 ts)
{
    auto &ch = m_imageChannels[cam];
    if (!ch) {
        auto res = foxglove::messages::CompressedImageChannel::create(("/camera/" + cam).toStdString());
        if (!res.has_value()) {
            qWarning() << "Image channel failed for" << cam << ":" << QString::fromStdString(foxglove::strerror(res.error()));
            m_imageChannels.remove(cam);
            return;
        }
        ch = std::make_shared<foxglove::messages::CompressedImageChannel>(std::move(res.value()));
    }

    foxglove::messages::CompressedImage img;
    img.timestamp = {static_cast<uint32_t>(ts / 1000000000ull), static_cast<uint32_t>(ts % 1000000000ull)};
    img.frame_id = cam.toStdString();
    img.format   = "jpeg";
    img.data.assign(reinterpret_cast<const std::byte*>(jpeg.constData()), reinterpret_cast<const std::byte*>(jpeg.constData()) + jpeg.size());
    ch->log(img, ts);
}

void FoxgloveSink::publishAudio(const QString &cam, const QByteArray &pcm, quint64 ts, quint32 rate, quint32 channels)
{
    auto &ch = m_audioChannels[cam];
    if (!ch) {
        auto res = foxglove::messages::RawAudioChannel::create(("/camera/" + cam + "/audio").toStdString());
        if (!res.has_value()) {
            qWarning() << "Audio channel failed for" << cam << ":" << QString::fromStdString(foxglove::strerror(res.error()));
            m_audioChannels.remove(cam);
            return;
        }
        ch = std::make_shared<foxglove::messages::RawAudioChannel>(std::move(res.value()));
    }

    foxglove::messages::RawAudio a;
    a.timestamp = {static_cast<uint32_t>(ts / 1000000000ull), static_cast<uint32_t>(ts % 1000000000ull)};
    a.format = "pcm-s16";
    a.sample_rate = rate;
    a.number_of_channels = channels;
    a.data.assign(reinterpret_cast<const std::byte*>(pcm.constData()), reinterpret_cast<const std::byte*>(pcm.constData()) + pcm.size());
    ch->log(a, ts);
}

void FoxgloveSink::startMcapLogging(bool enable, const QString &logDir)
{
    if (enable && !m_writer) {
        if (!QDir().mkpath(logDir)) {
            qWarning() << "Could not create log directory:" << logDir;
            return;
        }

        const QString path = QDir(logDir).absoluteFilePath( QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".mcap");
        const std::string pathStr = path.toStdString();

        foxglove::McapWriterOptions opts;
        opts.path = pathStr;

        auto res = foxglove::McapWriter::create(opts);
        if (res.has_value()) {
            m_writer = std::make_unique<foxglove::McapWriter>(std::move(res.value()));
            qInfo() << "MCAP logging started:" << path;
        } else {
            qWarning() << "MCAP create failed:" << QString::fromStdString(foxglove::strerror(res.error()));
        }
    } else if (!enable && m_writer) {
        m_writer.reset();
        qInfo() << "MCAP logging stopped.";
    }
}