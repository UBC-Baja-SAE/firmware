#include "webcam.h"
#include <QSocketNotifier>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
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

Webcam::Webcam(const QString &name, const QString &videoDev, const QString &audioDev, QObject *parent)
    : QObject(parent), m_name(name), m_videoDev(videoDev), m_audioDev(audioDev) {}
Webcam::~Webcam() { stop(); stopAudio(); }

void Webcam::begin()
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Webcam::scan);
    m_timer->start(1000);
    scan();
}

void Webcam::scan()
{
    if (m_fd < 0 && QFile::exists(m_videoDev))
        start();
    if (m_fd >= 0 && !m_pcm && !m_audioDev.isEmpty())
        startAudio();
}

bool Webcam::start()
{
    if (m_fd >= 0) return true;
    const int width = m_width, height = m_height, fps = m_fps;

    m_fd = ::open(m_videoDev.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) { qWarning() << "Webcam" << m_name << "open failed:" << m_videoDev; return false; }

    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    if (xioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0 ||
        fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG) {
        qWarning() << "Webcam" << m_name << "doesn't support MJPEG at that size";
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
    if (xioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        qWarning() << "Webcam" << m_name << "STREAMON failed:" << strerror(errno);
        stop(); return false;
    }

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &Webcam::onReadable);
    qInfo() << "Webcam" << m_name << "streaming MJPEG" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height;
    return true;
}

void Webcam::onReadable()
{
    for (;;) {
        v4l2_buffer b{};
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        if (xioctl(m_fd, VIDIOC_DQBUF, &b) < 0) {
            if (errno == EAGAIN) break;
            qWarning() << "Webcam" << m_name << "lost:" << strerror(errno);
            stop();
            stopAudio();
            return;
        }

        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        const quint64 tsNs = quint64(ts.tv_sec) * 1000000000ull + ts.tv_nsec;

        if (b.bytesused > 0 && !(b.flags & V4L2_BUF_FLAG_ERROR)) {
            emit frameReady(m_name,
                            QByteArray(static_cast<const char*>(m_bufs[b.index].start), b.bytesused),
                            tsNs);
        }
        xioctl(m_fd, VIDIOC_QBUF, &b);
    }
}

void Webcam::stop()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }
    if (m_fd >= 0) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(m_fd, VIDIOC_STREAMOFF, &type);
        for (auto &b : m_bufs) munmap(b.start, b.length);
        m_bufs.clear();
        ::close(m_fd); m_fd = -1;
    }
}

bool Webcam::startAudio()
{
    if (m_pcm) return true;
    const unsigned rate = m_audioRate, channels = m_audioChannels;

    int err = snd_pcm_open(&m_pcm, m_audioDev.toLocal8Bit().constData(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (err < 0) { qWarning() << "Audio" << m_name << "open failed:" << snd_strerror(err); m_pcm = nullptr; return false; }

    err = snd_pcm_set_params(m_pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 100000);
    if (err < 0) { qWarning() << "Audio" << m_name << "params failed:" << snd_strerror(err); stopAudio(); return false; }

    snd_pcm_uframes_t bufSize = 0, period = 0;
    snd_pcm_get_params(m_pcm, &bufSize, &period);
    m_period = period;
    m_rate = rate; m_channels = channels;
    m_audioBuf.resize(int(m_period * m_channels * sizeof(int16_t)));

    if ((err = snd_pcm_prepare(m_pcm)) < 0 || (err = snd_pcm_start(m_pcm)) < 0) {
        qWarning() << "Audio" << m_name << "start failed:" << snd_strerror(err); stopAudio(); return false;
    }

    const int n = snd_pcm_poll_descriptors_count(m_pcm);
    std::vector<pollfd> fds(n);
    snd_pcm_poll_descriptors(m_pcm, fds.data(), n);
    for (const auto &p : fds) {
        auto *sn = new QSocketNotifier(p.fd, QSocketNotifier::Read, this);
        connect(sn, &QSocketNotifier::activated, this, &Webcam::onAudioReadable);
        m_audioNotifiers.push_back(sn);
    }
    qInfo() << "Audio" << m_name << "capturing" << rate << "Hz," << channels << "ch, period" << m_period << "frames";
    return true;
}

void Webcam::onAudioReadable()
{
    if (!m_pcm) return;

    for (;;) {
        snd_pcm_sframes_t n = snd_pcm_readi(m_pcm, m_audioBuf.data(), m_period);
        if (n == -EAGAIN) return;
        if (n == -EPIPE || n == -ESTRPIPE) {
            qWarning() << "Audio" << m_name << "overrun, recovering";
            int r = snd_pcm_recover(m_pcm, int(n), 1);
            if (r < 0 || snd_pcm_start(m_pcm) < 0) {
                qWarning() << "Audio" << m_name << "recover failed:" << snd_strerror(r);
                stopAudio();
            }
            return;
        }
        if (n < 0) {
            qWarning() << "Audio" << m_name << "read error:" << snd_strerror(int(n));
            stopAudio();
            return;
        }
        if (n == 0) return;

        timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
        quint64 tsNs = quint64(ts.tv_sec) * 1000000000ull + ts.tv_nsec;
        tsNs -= quint64(n) * 1000000000ull / m_rate;

        emit audioReady(m_name,
                        QByteArray(m_audioBuf.constData(), int(n * m_channels * sizeof(int16_t))),
                        tsNs, m_rate, m_channels);
    }
}

void Webcam::stopAudio()
{
    for (auto *sn : m_audioNotifiers) { sn->setEnabled(false); sn->deleteLater(); }
    m_audioNotifiers.clear();
    if (m_pcm) { snd_pcm_close(m_pcm); m_pcm = nullptr; }
}