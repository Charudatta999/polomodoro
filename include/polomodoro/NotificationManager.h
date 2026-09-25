#pragma once

#include <QObject>
#include <QString>

class QDBusInterface;

namespace polomodoro {

class NotificationManager : public QObject {
    Q_OBJECT
public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    void notify(const QString &summary, const QString &body);

private:
    QDBusInterface *m_interface = nullptr;
    uint m_lastId = 0;
};

} // namespace polomodoro
