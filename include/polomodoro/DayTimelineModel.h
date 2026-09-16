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
    // Per spec the timeline is a fixed 24 * 72 px column, not fit-to-height, so
    // there is no dynamic hour window — only where to scroll on open.
    Q_PROPERTY(int scrollToMinutes READ scrollToMinutes NOTIFY contentChanged)
    Q_PROPERTY(int nowMinutes READ nowMinutes NOTIFY nowChanged)
    // QAbstractListModel exposes no count to QML on its own.
    Q_PROPERTY(int count READ count NOTIFY contentChanged)
    Q_PROPERTY(QVariantList unscheduled READ unscheduledTasks NOTIFY contentChanged)
public:
    enum Roles {
        TaskIdRole = Qt::UserRole + 1,
        TitleRole,
        KindRole,            // "planned" | "logged" | "running"
        StartMinutesRole,    // minutes from local midnight
        DurationMinutesRole,
        LabelRole,
        OverTargetRole,
        MovableRole,
        LaneRole,        // column index among blocks that overlap in time
        LaneCountRole,   // how many columns that overlap group needs
        CollapsedRole    // past the 3-lane cap: render as a 4 px tick
    };
    Q_ENUM(Roles)

    explicit DayTimelineModel(TaskTree &tree, QObject *parent = nullptr);

    QDate selectedDate() const;
    void setSelectedDate(const QDate &date);
    QString dayLabel() const;
    QString summaryLabel() const;
    QVariantList weekDays() const;
    bool showingToday() const;
    int scrollToMinutes() const;
    int nowMinutes() const;
    int count() const;

    Q_INVOKABLE void goToPreviousDay();
    Q_INVOKABLE void goToNextDay();
    Q_INVOKABLE void goToToday();
    Q_INVOKABLE void selectDate(const QDate &date);

    // Drops a dragged block at a new start offset, snapped to `snapMinutes`.
    // Only "planned" rows move; returns false for logged history.
    Q_INVOKABLE bool moveBlock(int row, int newStartMinutes, int snapMinutes = 5);

    // Tasks with no scheduled start have no time to render at, so they never
    // appear as blocks. They are surfaced separately so a newly added task is
    // visible here and can be dropped onto the day in one click.
    Q_INVOKABLE QVariantList unscheduledTasks() const;
    Q_INVOKABLE bool scheduleTaskAt(const QString &taskId, int startMinutes, int snapMinutes = 5);
    Q_INVOKABLE int suggestedStartMinutes() const;
    Q_INVOKABLE bool unscheduleTask(const QString &taskId);

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
        bool collapsed = false;
    };

    void assignLanes();

    TaskTree &m_tree;
    QDate m_date = QDate::currentDate();
    QVector<Block> m_blocks;
    int m_scrollToMinutes = 0;
    int m_activeCount = 0;
    qint64 m_loggedMsForDay = 0;

    void rebuild();
};

} // namespace polomodoro
