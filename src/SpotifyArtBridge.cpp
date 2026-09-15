#include "polomodoro/SpotifyArtBridge.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QWebEnginePage>

namespace polomodoro {

struct SpotifyArtBridge::Impl {
    QWebEnginePage *page = nullptr;
    QTimer pollTimer;
    QString artUrl;
    QString trackTitle;
    QString artistName;
    bool isPlaying = false;
};

static const char *kPollJs = R"(
(function() {
  const md = navigator.mediaSession && navigator.mediaSession.metadata;
  const state = navigator.mediaSession && navigator.mediaSession.playbackState;
  if (!md) return JSON.stringify({playing:false});
  const art = md.artwork && md.artwork.length ? md.artwork[md.artwork.length-1].src : '';
  return JSON.stringify({
    title: md.title || '',
    artist: md.artist || '',
    art: art,
    playing: state === 'playing'
  });
})()
)";

static const char *kPlayJs = R"(
(function(){
  const btn = document.querySelector('[data-testid="control-button-playpause"]');
  if (btn) { btn.click(); return true; }
  return false;
})()
)";

static const char *kPrevJs = R"(
(function(){
  const btn = document.querySelector('[data-testid="control-button-skip-back"]');
  if (btn) { btn.click(); return true; }
  return false;
})()
)";

static const char *kNextJs = R"(
(function(){
  const btn = document.querySelector('[data-testid="control-button-skip-forward"]');
  if (btn) { btn.click(); return true; }
  return false;
})()
)";

SpotifyArtBridge::SpotifyArtBridge(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>())
{
}

SpotifyArtBridge::~SpotifyArtBridge() = default;

void SpotifyArtBridge::attachWebPage(QWebEnginePage *page)
{
    d->page = page;
}

void SpotifyArtBridge::startPolling(int intervalMs)
{
    d->pollTimer.setInterval(intervalMs);
    connect(&d->pollTimer, &QTimer::timeout, this, [this]() {
        if (!d->page)
            return;
        d->page->runJavaScript(kPollJs, [this](const QVariant &result) {
            const auto doc = QJsonDocument::fromJson(result.toString().toUtf8());
            if (!doc.isObject())
                return;
            const QJsonObject obj = doc.object();
            d->trackTitle = obj.value(QStringLiteral("title")).toString();
            d->artistName = obj.value(QStringLiteral("artist")).toString();
            d->artUrl = obj.value(QStringLiteral("art")).toString();
            d->isPlaying = obj.value(QStringLiteral("playing")).toBool();
            emit metadataChanged();
        });
    });
    d->pollTimer.start();
}

QString SpotifyArtBridge::artUrl() const { return d->artUrl; }
QString SpotifyArtBridge::trackTitle() const { return d->trackTitle; }
QString SpotifyArtBridge::artistName() const { return d->artistName; }
QString SpotifyArtBridge::nowPlayingLabel() const
{
    if (d->trackTitle.isEmpty())
        return QStringLiteral("♪ —");
    if (d->artistName.isEmpty())
        return d->trackTitle;
    return d->trackTitle + QStringLiteral(" — ") + d->artistName;
}
bool SpotifyArtBridge::isPlaying() const { return d->isPlaying; }

void SpotifyArtBridge::play() { if (d->page) d->page->runJavaScript(kPlayJs); }
void SpotifyArtBridge::pause() { if (d->page) d->page->runJavaScript(kPlayJs); }
void SpotifyArtBridge::togglePlayPause() { play(); }
void SpotifyArtBridge::next() { if (d->page) d->page->runJavaScript(kNextJs); }
void SpotifyArtBridge::previous() { if (d->page) d->page->runJavaScript(kPrevJs); }

} // namespace polomodoro
