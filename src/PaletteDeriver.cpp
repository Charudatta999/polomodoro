#include "polomodoro/PaletteDeriver.h"

#include "polomodoro/SettingsStore.h"

#include <QImage>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVector>
#include <QtMath>
#include <cmath>

namespace polomodoro {

Q_LOGGING_CATEGORY(lcPalette, "polomodoro.palette")

namespace {

struct Lab {
    float L = 0;
    float a = 0;
    float b = 0;
};

float srgbToLinear(float c)
{
    c = qBound(0.f, c, 1.f);
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

float linearToSrgb(float c)
{
    c = qBound(0.f, c, 1.f);
    return c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1.f / 2.4f) - 0.055f;
}

Lab rgbToOklab(float r, float g, float b)
{
    const float lr = srgbToLinear(r);
    const float lg = srgbToLinear(g);
    const float lb = srgbToLinear(b);
    const float l = 0.4122214708f * lr + 0.5363325363f * lg + 0.0514459929f * lb;
    const float m = 0.2119034982f * lr + 0.6806995451f * lg + 0.1073969566f * lb;
    const float s = 0.0883024619f * lr + 0.2817188376f * lg + 0.6299787005f * lb;
    const float l_ = std::cbrt(l);
    const float m_ = std::cbrt(m);
    const float s_ = std::cbrt(s);
    return {0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
            1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
            0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_};
}

QColor oklabToColor(Lab lab)
{
    const float l_ = lab.L + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
    const float m_ = lab.L - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
    const float s_ = lab.L - 0.0894841775f * lab.a - 1.2914855480f * lab.b;
    const float l = l_ * l_ * l_;
    const float m = m_ * m_ * m_;
    const float s = s_ * s_ * s_;
    const float r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    const float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    const float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
    return QColor::fromRgbF(linearToSrgb(r), linearToSrgb(g), linearToSrgb(b));
}

struct Lch {
    float L = 0;
    float C = 0;
    float h = 0;
};

Lch labToLch(Lab lab)
{
    Lch o;
    o.L = lab.L;
    o.C = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    o.h = std::atan2(lab.b, lab.a) * 180.f / float(M_PI);
    if (o.h < 0)
        o.h += 360.f;
    return o;
}

Lab lchToLab(Lch lch)
{
    const float rad = lch.h * float(M_PI) / 180.f;
    return {lch.L, lch.C * std::cos(rad), lch.C * std::sin(rad)};
}

Lch clampSpec(Lch c)
{
    c.L = qBound(0.62f, c.L, 0.82f);
    c.C = qBound(0.09f, c.C, 0.22f);
    while (c.h < 0)
        c.h += 360.f;
    while (c.h >= 360.f)
        c.h -= 360.f;
    return c;
}

QColor fromLch(Lch c)
{
    return oklabToColor(lchToLab(c));
}

QColor deriveFromImage(const QImage &src)
{
    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    img = img.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QVector<Lab> samples;
    samples.reserve(64 * 64);
    for (int y = 0; y < img.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgb p = line[x];
            if (qAlpha(p) < 128)
                continue;
            Lab lab = rgbToOklab(qRed(p) / 255.f, qGreen(p) / 255.f, qBlue(p) / 255.f);
            if (lab.L < 0.08f || lab.L > 0.95f)
                continue;
            samples.push_back(lab);
        }
    }
    if (samples.isEmpty())
        return QColor(QStringLiteral("#1ED760"));

    constexpr int k = 5;
    QVector<Lab> means = samples.mid(0, k);
    if (means.size() < k) {
        while (means.size() < k)
            means.push_back(samples.first());
    }
    QVector<int> assign(samples.size(), 0);
    for (int iter = 0; iter < 8; ++iter) {
        for (int i = 0; i < samples.size(); ++i) {
            float best = 1e9f;
            int bi = 0;
            for (int c = 0; c < k; ++c) {
                const float dL = samples[i].L - means[c].L;
                const float da = samples[i].a - means[c].a;
                const float db = samples[i].b - means[c].b;
                const float d = dL * dL + da * da + db * db;
                if (d < best) {
                    best = d;
                    bi = c;
                }
            }
            assign[i] = bi;
        }
        QVector<Lab> sum(k);
        QVector<int> count(k, 0);
        for (int i = 0; i < samples.size(); ++i) {
            sum[assign[i]].L += samples[i].L;
            sum[assign[i]].a += samples[i].a;
            sum[assign[i]].b += samples[i].b;
            count[assign[i]]++;
        }
        for (int c = 0; c < k; ++c) {
            if (count[c] == 0)
                continue;
            means[c].L = sum[c].L / float(count[c]);
            means[c].a = sum[c].a / float(count[c]);
            means[c].b = sum[c].b / float(count[c]);
        }
    }

    int best = 0;
    float bestC = -1;
    const int minCount = qMax(1, samples.size() / 50);
    QVector<int> counts(k, 0);
    for (int a : assign)
        counts[a]++;
    for (int c = 0; c < k; ++c) {
        if (counts[c] < minCount)
            continue;
        const float chroma = std::sqrt(means[c].a * means[c].a + means[c].b * means[c].b);
        if (chroma > bestC) {
            bestC = chroma;
            best = c;
        }
    }
    return fromLch(clampSpec(labToLch(means[best])));
}

} // namespace

struct PaletteDeriver::Impl {
    SettingsStore &settings;
    QNetworkAccessManager net;
    QColor accent{QStringLiteral("#1ED760")};
    QColor accentHover{QStringLiteral("#3BE377")};
    QColor breakColor{QStringLiteral("#58C8B0")};
    QColor overflow{QStringLiteral("#E8C547")};
    QColor muted{QStringLiteral("#6B7F73")};
    QString lastUrl;
    QString inFlightUrl;

    explicit Impl(SettingsStore &s) : settings(s) {}
};

PaletteDeriver::PaletteDeriver(SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(settings))
{
}

PaletteDeriver::~PaletteDeriver() = default;

QColor PaletteDeriver::accent() const { return d->accent; }
QColor PaletteDeriver::accentHover() const { return d->accentHover; }
QColor PaletteDeriver::breakColor() const { return d->breakColor; }
QColor PaletteDeriver::overflow() const { return d->overflow; }
QColor PaletteDeriver::muted() const { return d->muted; }

void PaletteDeriver::applyForest()
{
    applyDerived(QColor(QStringLiteral("#1ED760")));
}

void PaletteDeriver::applyManual(const QString &presetId)
{
    if (presetId == QLatin1String("ocean"))
        applyDerived(QColor(QStringLiteral("#58C8B0")));
    else if (presetId == QLatin1String("ember"))
        applyDerived(QColor(QStringLiteral("#E8C547")));
    else if (presetId == QLatin1String("rose"))
        applyDerived(QColor(QStringLiteral("#E8489B")));
    else
        applyDerived(QColor(QStringLiteral("#1ED760")));
}

void PaletteDeriver::applyDerived(const QColor &accent)
{
    Lch base = clampSpec(labToLch(rgbToOklab(float(accent.redF()), float(accent.greenF()), float(accent.blueF()))));
    Lch hover = base;
    hover.L = qBound(0.62f, base.L + 0.06f, 0.88f);
    Lch brk = base;
    brk.h = std::fmod(base.h + 48.f, 360.f);
    Lch over = base;
    over.h = 95.f;
    Lch mut = base;
    mut.C = 0.04f;
    mut.L = 0.45f;

    d->accent = fromLch(base);
    d->accentHover = fromLch(hover);
    d->breakColor = fromLch(clampSpec(brk));
    d->overflow = fromLch(clampSpec(over));
    d->muted = oklabToColor(lchToLab(mut));
    qCInfo(lcPalette) << "applied accent" << d->accent.name()
                      << "hover" << d->accentHover.name()
                      << "break" << d->breakColor.name()
                      << "overflow" << d->overflow.name()
                      << "muted" << d->muted.name();
    emit paletteChanged();
}

void PaletteDeriver::recompute(const QString &imageUrl)
{
    const QString mode = d->settings.getString(QStringLiteral("accentMode"), QStringLiteral("auto"));
    if (mode == QLatin1String("manual")) {
        const QString preset = d->settings.getString(QStringLiteral("accentManualPalette"), QStringLiteral("forest"));
        qCInfo(lcPalette) << "manual preset" << preset;
        applyManual(preset);
        return;
    }
    if (imageUrl.isEmpty() || imageUrl.startsWith(QLatin1String("gradient://"))) {
        qCInfo(lcPalette) << "no image — forest default";
        applyForest();
        return;
    }
    if (imageUrl == d->lastUrl)
        return;
    qCInfo(lcPalette) << "recompute mode" << mode << "url" << imageUrl;

    const QUrl url(imageUrl);
    if (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https")) {
        d->inFlightUrl = imageUrl;
        qCInfo(lcPalette) << "fetching" << imageUrl;
        auto *reply = d->net.get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, this, [this, reply, imageUrl]() {
            reply->deleteLater();
            if (d->inFlightUrl != imageUrl) {
                qCInfo(lcPalette) << "stale fetch dropped" << imageUrl;
                return;
            }
            if (reply->error() != QNetworkReply::NoError)
                qCWarning(lcPalette) << "fetch failed" << reply->errorString() << imageUrl;
            QImage img;
            img.loadFromData(reply->readAll());
            if (img.isNull()) {
                qCWarning(lcPalette) << "image decode failed" << imageUrl << "— forest default";
                applyForest();
                return;
            }
            qCInfo(lcPalette) << "decoded" << img.size() << imageUrl;
            d->lastUrl = imageUrl;
            applyDerived(deriveFromImage(img));
        });
        return;
    }

    QImage img;
    const QString path = url.scheme() == QLatin1String("qrc")
        ? QStringLiteral(":") + url.path()
        : url.toLocalFile();
    qCInfo(lcPalette) << "load local" << (path.isEmpty() ? imageUrl : path);
    if (!img.load(path.isEmpty() ? imageUrl : path)) {
        qCWarning(lcPalette) << "load failed — forest default";
        applyForest();
        return;
    }
    qCInfo(lcPalette) << "decoded" << img.size() << imageUrl;
    d->lastUrl = imageUrl;
    applyDerived(deriveFromImage(img));
}

} // namespace polomodoro
