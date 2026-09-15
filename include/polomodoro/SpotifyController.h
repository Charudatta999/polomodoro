#pragma once

#include <QObject>

namespace polomodoro {

class SpotifyArtBridge;

class SpotifyController : public QObject {
    Q_OBJECT
    Q_PROPERTY(SpotifyArtBridge *bridge READ bridge CONSTANT)
public:
    explicit SpotifyController(SpotifyArtBridge &bridge, QObject *parent = nullptr);

    SpotifyArtBridge *bridge() const;

private:
    SpotifyArtBridge &m_bridge;
};

} // namespace polomodoro
