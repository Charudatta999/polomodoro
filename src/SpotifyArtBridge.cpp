#include "polomodoro/SpotifyArtBridge.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QWebEnginePage>

namespace polomodoro {

struct SpotifyArtBridge::Impl {
    QWebEnginePage *page = nullptr;
    QTimer pollTimer;
    bool pollConnected = false;
    QString artUrl;
    QString trackTitle;
    QString artistName;
    bool isPlaying = false;
};

static const char *kPollJs = R"(
(function() {
  const md = navigator.mediaSession && navigator.mediaSession.metadata;
  const state = navigator.mediaSession && navigator.mediaSession.playbackState;
  if (md && md.title) {
    const art = md.artwork && md.artwork.length ? md.artwork[md.artwork.length - 1].src : '';
    return JSON.stringify({
      title: md.title || '',
      artist: md.artist || '',
      art: art,
      playing: state === 'playing'
    });
  }

  const playBtn = document.querySelector('[data-testid="control-button-playpause"]');
  const playing = !!(playBtn && playBtn.getAttribute('aria-label')
    && playBtn.getAttribute('aria-label').toLowerCase().includes('pause'));
  const titleEl = document.querySelector('[data-testid="context-item-info-title"]')
    || document.querySelector('[data-testid="now-playing-widget"] [dir="auto"]')
    || document.querySelector('a[data-testid="context-item-link"]');
  const artistEl = document.querySelector('[data-testid="context-item-info-artist"]')
    || document.querySelector('[data-testid="now-playing-widget"] + div [dir="auto"]');
  const artImg = document.querySelector('[data-testid="now-playing-widget"] img')
    || document.querySelector('img[data-testid="cover-art"]');

  return JSON.stringify({
    title: titleEl ? titleEl.textContent.trim() : '',
    artist: artistEl ? artistEl.textContent.trim() : '',
    art: artImg ? artImg.src : '',
    playing: playing
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
    if (!d->pollConnected) {
        connect(&d->pollTimer, &QTimer::timeout, this, [this]() {
            if (!d->page)
                return;
            d->page->runJavaScript(kPollJs, [this](const QVariant &result) {
                const auto doc = QJsonDocument::fromJson(result.toString().toUtf8());
                if (!doc.isObject())
                    return;
                const QJsonObject obj = doc.object();
                const QString title = obj.value(QStringLiteral("title")).toString();
                const QString artist = obj.value(QStringLiteral("artist")).toString();
                const QString art = obj.value(QStringLiteral("art")).toString();
                const bool playing = obj.value(QStringLiteral("playing")).toBool();
                if (title == d->trackTitle && artist == d->artistName && art == d->artUrl
                    && playing == d->isPlaying)
                    return;
                d->trackTitle = title;
                d->artistName = artist;
                d->artUrl = art;
                d->isPlaying = playing;
                emit metadataChanged();
            });
        });
        d->pollConnected = true;
    }
    if (!d->pollTimer.isActive())
        d->pollTimer.start();
}

QString SpotifyArtBridge::artUrl() const { return d->artUrl; }
QString SpotifyArtBridge::trackTitle() const { return d->trackTitle; }
QString SpotifyArtBridge::artistName() const { return d->artistName; }
QString SpotifyArtBridge::nowPlayingLabel() const
{
    if (d->trackTitle.isEmpty())
        return {};
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
