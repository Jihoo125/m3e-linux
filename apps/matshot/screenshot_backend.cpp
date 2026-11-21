#include "screenshot_backend.hpp"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QImage>
#include <QStandardPaths>
#include <QUrl>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <wayland-client.h>
#include "wlr-screencopy-unstable-v1-client-protocol.h"

namespace mat {
namespace {

struct CaptureState {
    wl_shm *shm = nullptr;
    zwlr_screencopy_manager_v1 *manager = nullptr;
    wl_output *output = nullptr;
    zwlr_screencopy_frame_v1 *frame = nullptr;
    uint32_t format = 0, width = 0, height = 0, stride = 0;
    bool bufferDone = false, ready = false, failed = false;
    void *pixels = nullptr;
    size_t pixelsSize = 0;
};

void registryGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    auto *s = static_cast<CaptureState *>(data);
    if (strcmp(interface, wl_shm_interface.name) == 0)
        s->shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
    else if (strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0)
        s->manager = static_cast<zwlr_screencopy_manager_v1 *>(wl_registry_bind(registry, name, &zwlr_screencopy_manager_v1_interface, qMin(version, 3u)));
    else if (strcmp(interface, wl_output_interface.name) == 0 && !s->output)
        s->output = static_cast<wl_output *>(wl_registry_bind(registry, name, &wl_output_interface, 1));
}
void registryRemove(void *, wl_registry *, uint32_t) {}
const wl_registry_listener registryListener = {registryGlobal, registryRemove};

void frameBuffer(void *data, zwlr_screencopy_frame_v1 *, uint32_t format, uint32_t width, uint32_t height, uint32_t stride)
{
    auto *s = static_cast<CaptureState *>(data);
    s->format = format; s->width = width; s->height = height; s->stride = stride;
}
void frameFlags(void *, zwlr_screencopy_frame_v1 *, uint32_t) {}
void frameReady(void *data, zwlr_screencopy_frame_v1 *, uint32_t, uint32_t, uint32_t)
{
    static_cast<CaptureState *>(data)->ready = true;
}
void frameFailed(void *data, zwlr_screencopy_frame_v1 *)
{
    static_cast<CaptureState *>(data)->failed = true;
}
void frameDamage(void *, zwlr_screencopy_frame_v1 *, uint32_t, uint32_t, uint32_t, uint32_t) {}
void frameLinuxDmabuf(void *, zwlr_screencopy_frame_v1 *, uint32_t, uint32_t, uint32_t)
{
    // We only use shm buffers; ignore dmabuf offers.
}
void frameBufferDone(void *data, zwlr_screencopy_frame_v1 *)
{
    static_cast<CaptureState *>(data)->bufferDone = true;
}
// Field order must match the generated protocol struct exactly:
// buffer, flags, ready, failed, damage, linux_dmabuf, buffer_done.
const zwlr_screencopy_frame_v1_listener frameListener = {
    frameBuffer, frameFlags, frameReady, frameFailed, frameDamage, frameLinuxDmabuf, frameBufferDone};

QString captureOnce(QString *error)
{
    wl_display *display = wl_display_connect(nullptr);
    if (!display) { *error = QStringLiteral("No Wayland display (run inside the session)"); return {}; }
    CaptureState state;
    wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, &state);
    wl_display_roundtrip(display);
    if (!state.shm || !state.manager || !state.output) {
        wl_display_disconnect(display);
        *error = QStringLiteral("Compositor lacks screencopy support");
        return {};
    }
    state.frame = zwlr_screencopy_manager_v1_capture_output(state.manager, 0, state.output);
    zwlr_screencopy_frame_v1_add_listener(state.frame, &frameListener, &state);
    while (!state.bufferDone && !state.failed) wl_display_roundtrip(display);
    if (state.failed || state.width == 0) {
        zwlr_screencopy_frame_v1_destroy(state.frame);
        wl_display_disconnect(display);
        *error = QStringLiteral("Capture failed");
        return {};
    }
    const size_t size = static_cast<size_t>(state.stride) * state.height;
    const int fd = memfd_create("matshot", MFD_CLOEXEC);
    if (fd < 0 || ftruncate(fd, static_cast<off_t>(size)) != 0) {
        if (fd >= 0) close(fd);
        zwlr_screencopy_frame_v1_destroy(state.frame);
        wl_display_disconnect(display);
        *error = QStringLiteral("Cannot allocate capture buffer");
        return {};
    }
    state.pixels = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    state.pixelsSize = size;
    wl_shm_pool *pool = wl_shm_create_pool(state.shm, fd, static_cast<int32_t>(size));
    wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, static_cast<int32_t>(state.width), static_cast<int32_t>(state.height),
                                                  static_cast<int32_t>(state.stride), state.format);
    zwlr_screencopy_frame_v1_copy(state.frame, buffer);
    while (!state.ready && !state.failed) wl_display_roundtrip(display);
    QString path;
    if (state.ready && !state.failed) {
        QImage image(static_cast<const uchar *>(state.pixels), static_cast<int>(state.width), static_cast<int>(state.height),
                     static_cast<int>(state.stride),
                     state.format == WL_SHM_FORMAT_ARGB8888 ? QImage::Format_ARGB32 : QImage::Format_RGB32);
        const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QStringLiteral("/Screenshots");
        QDir().mkpath(dirPath);
        path = dirPath + QStringLiteral("/matshot-%1.png").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
        if (!image.copy(0, 0, image.width(), image.height()).save(path)) {
            *error = QStringLiteral("Cannot save %1").arg(path);
            path.clear();
        }
    } else {
        *error = QStringLiteral("Compositor refused the capture");
    }
    munmap(state.pixels, size);
    wl_buffer_destroy(buffer);
    wl_shm_pool_destroy(pool);
    close(fd);
    zwlr_screencopy_frame_v1_destroy(state.frame);
    wl_display_disconnect(display);
    return path;
}

} // namespace

ScreenshotBackend::ScreenshotBackend(QObject *parent) : QObject(parent) {}

void ScreenshotBackend::capture()
{
    if (m_busy) return;
    m_busy = true;
    emit busyChanged();
    std::thread([this] {
        QString error;
        const QString path = captureOnce(&error);
        // Marshal back to the GUI thread.
        QMetaObject::invokeMethod(this, [this, path, error] { finish(path, error); }, Qt::QueuedConnection);
    }).detach();
}

void ScreenshotBackend::finish(const QString &path, const QString &error)
{
    m_busy = false;
    if (!path.isEmpty()) {
        m_lastPath = path;
        m_status = QStringLiteral("Saved %1").arg(path);
    } else {
        m_status = error;
    }
    emit busyChanged();
    emit captured(path, error);
}

void ScreenshotBackend::openFolder()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QStringLiteral("/Screenshots");
    QDir().mkpath(dirPath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
}

} // namespace mat
