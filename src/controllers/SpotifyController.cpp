#include "polomodoro/SpotifyController.h"

#include "polomodoro/SpotifyArtBridge.h"

#include <QWebEnginePage>

namespace polomodoro {

SpotifyController::SpotifyController(SpotifyArtBridge &bridge, QObject *parent)
    : QObject(parent), m_bridge(bridge)
{
    connect(&m_bridge, &SpotifyArtBridge::metadataChanged, this, &SpotifyController::playbackChanged);
}

bool SpotifyController::ready() const { return m_ready; }
bool SpotifyController::isPlaying() const { return m_bridge.isPlaying(); }
QString SpotifyController::nowPlayingLabel() const { return m_bridge.nowPlayingLabel(); }
bool SpotifyController::premiumRequired() const { return m_premiumRequired; }
SpotifyArtBridge *SpotifyController::bridge() const { return const_cast<SpotifyArtBridge *>(&m_bridge); }

void SpotifyController::attach(QObject *webView)
{
    if (!webView)
        return;

    QWebEnginePage *page = qvariant_cast<QWebEnginePage *>(webView->property("page"));
    if (!page)
        return;

    m_bridge.attachWebPage(page);
    m_bridge.startPolling(2000);
    if (!m_ready) {
        m_ready = true;
        emit readyChanged();
    }
}

void SpotifyController::play() { m_bridge.play(); }
void SpotifyController::pause() { m_bridge.pause(); }
void SpotifyController::togglePlayPause() { m_bridge.togglePlayPause(); }
void SpotifyController::next() { m_bridge.next(); }
void SpotifyController::previous() { m_bridge.previous(); }

} // namespace polomodoro
