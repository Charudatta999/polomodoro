#pragma once

#include <QAbstractListModel>
#include <QDate>
#include <QVariantList>

#include "polomodoro/TaskTree.h"

namespace polomodoro {

// Blocks on the day timeline for one calendar day, in local time.
//
// Two kinds share the list: "planned" blocks come from a task's scheduled
// window and can be dragged to reschedule; "logged" blocks come from the
// sessions table and are history, so they are fixed.
class DayTimelineModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY selectedDateChanged)
    Q_PROPERTY(QString dayLabel READ dayLabel NOTIFY selectedDateChanged)
    Q_PROPERTY(QString summaryLabel READ summaryLabel NOTIFY contentChanged)
    Q_PROPERTY(QVariantList weekDays READ weekDays NOTIFY selectedDateChanged)
    Q_PROPERTY(bool showingToday READ showingToday NOTIFY selectedDateChanged)
    Q_PROPERTY(int firstHour READ firstHour NOTIFY contentChanged)
    Q_PROPERTY(int hourCount READ hourCount NOTIFY contentChanged)
    Q_PROPERTY(int nowMinutes READ nowMinutes NOTIFY nowChanged)
    // QAbstractListModel exposes no count to QML on its own.
    Q_PROPERTY(int count READ count NOTIFY contentChanged)
public:
    enum Roles {
        TaskIdRole = Qt::UserRole + 1,
        TitleRole,
        KindRole,            // "planned" | "logged"
        StartMinutesRole,    // minutes from local midnight
        DurationMinutesRole,
        LabelRole,
        OverTargetRole,
        MovableRole,
        LaneRole,        // column index among blocks that overlap in time
        LaneCountRole    // how many columns that overlap group needs
    };
    Q_ENUM(Roles)

    explicit DayTimelineModel(TaskTree &tree, QObject *parent = nullptr);

    QDate selectedDate() const;
    void setSelectedDate(const QDate &date);
    QString dayLabel() const;
    QString summaryLabel() const;
    QVariantList weekDays() const;
    bool showingToday() const;
    int firstHour() const;
    int hourCount() const;
    int nowMinutes() const;
    int count() const;

    Q_INVOKABLE void goToPreviousDay();
    Q_INVOKABLE void goToNextDay();
    Q_INVOKABLE void goToToday();
    Q_INVOKABLE void selectDate(const QDate &date);

    // Drops a dragged block at a new start offset, snapped to `snapMinutes`.
    // Only "planned" rows move; returns false for logged history.
    Q_INVOKABLE bool moveBlock(int row, int newStartMinutes, int snapMinutes = 5);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();
    void tick();

signals:
    void selectedDateChanged();
    void contentChanged();
    void nowChanged();

private:
    struct Block {
        QString taskId;
        QString title;
        QString kind;
        int startMinutes = 0;
        int durationMinutes = 0;
        QString label;
        bool overTarget = false;
        bool movable = false;
        int lane = 0;
        int laneCount = 1;
    };

    void assignLanes();

    TaskTree &m_tree;
    QDate m_date = QDate::currentDate();
    QVector<Block> m_blocks;
    int m_firstHour = 9;
    int m_hourCount = 6;
    qint64 m_loggedMsForDay = 0;

    void rebuild();
};

} // namespace polomodoro
