#include "webcam.h"
#include <QSocketNotifier>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <alsa/asoundlib.h>

static int xioctl(int fd, unsigned long req, void *arg)
{
    int r;
    do { r = ioctl(fd, req, arg); } while (r == -1 && errno == EINTR);
    return r;
}

Webcam::Webcam(QObject *parent) : QObject(parent) {}
Webcam::~Webcam() { stop(); stopAudio(); }

bool Webcam::start(const QString &device, int width, int height, int fps)
{
    m_fd = ::open(device.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) { qWarning() << "Webcam open failed:" << device; return false; }

    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    if (xioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0 ||
        fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG) {
        qWarning() << "Webcam doesn't support MJPEG at that size";
        stop(); return false;
    }

    v4l2_streamparm parm{};
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe = {1, static_cast<__u32>(fps)};
    xioctl(m_fd, VIDIOC_S_PARM, &parm);

    v4l2_requestbuffers req{};
    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) { stop(); return false; }

    for (unsigned i = 0; i < req.count; ++i) {
        v4l2_buffer b{};
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        b.index = i;
        if (xioctl(m_fd, VIDIOC_QUERYBUF, &b) < 0) { stop(); return false; }
        void *p = mmap(nullptr, b.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, b.m.offset);
        if (p == MAP_FAILED) { stop(); return false; }
        m_bufs.push_back({p, b.length});
        xioctl(m_fd, VIDIOC_QBUF, &b);
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_STREAMON, &type) < 0) { stop(); return false; }

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &Webcam::onReadable);
    qInfo() << "Webcam streaming MJPEG" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height;
    return true;
}

void Webcam::onReadable()
{
    for (;;) {
        v4l2_buffer b{};
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        if (xioctl(m_fd, VIDIOC_DQBUF, &b) < 0) break;

        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        const quint64 tsNs = quint64(ts.tv_sec) * 1000000000ull + ts.tv_nsec;

        if (b.bytesused > 0) {
            emit frameReady(QByteArray(static_cast<const char*>(m_bufs[b.index].start),
                                       b.bytesused), tsNs);
        }
        xioctl(m_fd, VIDIOC_QBUF, &b);
    }
}

void Webcam::stop()
{
    delete m_notifier; m_notifier = nullptr;
    if (m_fd >= 0) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(m_fd, VIDIOC_STREAMOFF, &type);
        for (auto &b : m_bufs) munmap(b.start, b.length);
        m_bufs.clear();
        ::close(m_fd); m_fd = -1;
    }
}

bool Webcam::startAudio(const QString &device, unsigned rate, unsigned channels)
{
    int err = snd_pcm_open(&m_pcm, device.toLocal8Bit().constData(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (err < 0) { qWarning() << "Audio open failed:" << snd_strerror(err); m_pcm = nullptr; return false; }

    err = snd_pcm_set_params(m_pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 100000);
    if (err < 0) { qWarning() << "Audio params failed:" << snd_strerror(err); stopAudio(); return false; }

    snd_pcm_uframes_t bufSize = 0, period = 0;
    snd_pcm_get_params(m_pcm, &bufSize, &period);
    m_period = period;
    m_rate = rate; m_channels = channels;
    m_audioBuf.resize(int(m_period * m_channels * sizeof(int16_t)));

    if ((err = snd_pcm_prepare(m_pcm)) < 0 || (err = snd_pcm_start(m_pcm)) < 0) {
        qWarning() << "Audio start failed:" << snd_strerror(err); stopAudio(); return false;
    }

    const int n = snd_pcm_poll_descriptors_count(m_pcm);
    std::vector<pollfd> fds(n);
    snd_pcm_poll_descriptors(m_pcm, fds.data(), n);
    for (const auto &p : fds) {
        auto *sn = new QSocketNotifier(p.fd, QSocketNotifier::Read, this);
        connect(sn, &QSocketNotifier::activated, this, &Webcam::onAudioReadable);
        m_audioNotifiers.push_back(sn);
    }
    qInfo() << "Audio capturing" << rate << "Hz," << channels << "ch, period" << m_period << "frames";
    return true;
}

void Webcam::onAudioReadable()
{
    for (;;) {
        snd_pcm_sframes_t n = snd_pcm_readi(m_pcm, m_audioBuf.data(), m_period);
        if (n == -EAGAIN) return;
        if (n == -EPIPE || n == -ESTRPIPE) {
            qWarning() << "Audio overrun, recovering";
            snd_pcm_recover(m_pcm, int(n), 1);
            snd_pcm_start(m_pcm);
            return;
        }
        if (n < 0) { qWarning() << "Audio read error:" << snd_strerror(int(n)); return; }
        if (n == 0) return;

        timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
        quint64 tsNs = quint64(ts.tv_sec) * 1000000000ull + ts.tv_nsec;
        tsNs -= quint64(n) * 1000000000ull / m_rate;   // timestamp = start of chunk

        emit audioReady(QByteArray(m_audioBuf.constData(), int(n * m_channels * sizeof(int16_t))), tsNs, m_rate, m_channels);
    }
}

void Webcam::stopAudio()
{
    qDeleteAll(m_audioNotifiers); m_audioNotifiers.clear();
    if (m_pcm) { snd_pcm_close(m_pcm); m_pcm = nullptr; }
}