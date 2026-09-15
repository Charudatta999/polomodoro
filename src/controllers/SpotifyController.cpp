#include "polomodoro/SpotifyController.h"

#include "polomodoro/SpotifyArtBridge.h"

namespace polomodoro {

SpotifyController::SpotifyController(SpotifyArtBridge &bridge, QObject *parent)
    : QObject(parent), m_bridge(bridge)
{
}

SpotifyArtBridge *SpotifyController::bridge() const { return const_cast<SpotifyArtBridge *>(&m_bridge); }

} // namespace polomodoro
