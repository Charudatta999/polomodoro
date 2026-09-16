#include "polomodoro/DayTimelineModel.h"

#include <QLocale>
#include <algorithm>

namespace polomodoro {

namespace {

QString durationLabel(qint64 ms)
{
    const qint64 totalMin = ms / 60000;
    const qint64 h = totalMin / 60;
    const qint64 m = totalMin % 60;
    if (h > 0)
        return QStringLiteral("%1h %2m").arg(h).arg(m);
    return QStringLiteral("%1m").arg(m);
}

int minutesFromMidnight(const QDateTime &localDt)
{
    return localDt.time().hour() * 60 + localDt.time().minute();
}

} // namespace

DayTimelineModel::DayTimelineModel(TaskTree &tree, QObject *parent)
    : QAbstractListModel(parent), m_tree(tree)
{
    rebuild();
}

QDate DayTimelineModel::selectedDate() const { return m_date; }

void DayTimelineModel::setSelectedDate(const QDate &date)
{
    if (!date.isValid() || date == m_date)
        return;
    m_date = date;
    refresh();
    emit selectedDateChanged();
}

QString DayTimelineModel::dayLabel() const
{
    return QLocale().toString(m_date, QStringLiteral("dddd, d MMMM"));
}

QString DayTimelineModel::summaryLabel() const
{
    int active = 0;
    for (const Block &b : m_blocks) {
        if (b.kind == QLatin1String("planned"))
            ++active;
    }
    return QStringLiteral("%1 PLANNED · %2 LOGGED")
        .arg(active)
        .arg(m_loggedMsForDay > 0 ? durationLabel(m_loggedMsForDay) : QStringLiteral("0M"))
        .toUpper();
}

QVariantList DayTimelineModel::weekDays() const
{
    // Seven days centred on the selection, so stepping never leaves the strip
    // without a highlighted cell.
    QVariantList out;
    const QDate today = QDate::currentDate();
    for (int i = -3; i <= 3; ++i) {
        const QDate d = m_date.addDays(i);
        QVariantMap m;
        m[QStringLiteral("date")] = d;
        m[QStringLiteral("day")] = QString::number(d.day());
        m[QStringLiteral("weekday")] = QLocale().toString(d, QStringLiteral("ddd")).toUpper();
        m[QStringLiteral("isSelected")] = (d == m_date);
        m[QStringLiteral("isToday")] = (d == today);
        out.push_back(m);
    }
    return out;
}

bool DayTimelineModel::showingToday() const { return m_date == QDate::currentDate(); }
int DayTimelineModel::firstHour() const { return m_firstHour; }
int DayTimelineModel::hourCount() const { return m_hourCount; }

int DayTimelineModel::count() const { return m_blocks.size(); }

int DayTimelineModel::nowMinutes() const
{
    const QTime t = QTime::currentTime();
    return t.hour() * 60 + t.minute();
}

void DayTimelineModel::goToPreviousDay() { setSelectedDate(m_date.addDays(-1)); }
void DayTimelineModel::goToNextDay() { setSelectedDate(m_date.addDays(1)); }
void DayTimelineModel::goToToday() { setSelectedDate(QDate::currentDate()); }
void DayTimelineModel::selectDate(const QDate &date) { setSelectedDate(date); }

bool DayTimelineModel::moveBlock(int row, int newStartMinutes, int snapMinutes)
{
    if (row < 0 || row >= m_blocks.size())
        return false;
    const Block &b = m_blocks.at(row);
    if (!b.movable)
        return false;

    if (snapMinutes < 1)
        snapMinutes = 1;
    int snapped = ((newStartMinutes + snapMinutes / 2) / snapMinutes) * snapMinutes;
    snapped = std::clamp(snapped, 0, 24 * 60 - 1);

    // Interpret the drop in the displayed day's local time, then store UTC.
    const QDateTime localStart(m_date, QTime(snapped / 60, snapped % 60));
    if (!m_tree.rescheduleTask(b.taskId, localStart.toUTC()))
        return false;

    refresh();
    return true;
}

QVariantList DayTimelineModel::unscheduledTasks() const
{
    QVariantList out;
    for (const TaskNode *task : m_tree.allTasks()) {
        if (!task || task->scheduledStartAt.isValid())
            continue;
        if (task->status == TaskStatus::Completed)
            continue;
        QVariantMap m;
        m[QStringLiteral("taskId")] = task->id;
        m[QStringLiteral("title")] = task->title;
        out.push_back(m);
    }
    return out;
}

int DayTimelineModel::suggestedStartMinutes() const
{
    // Next quarter hour when looking at today, otherwise the start of the
    // working range so the block lands somewhere visible.
    if (!showingToday())
        return 9 * 60;
    const int now = nowMinutes();
    return std::min(((now / 15) + 1) * 15, 24 * 60 - 15);
}

bool DayTimelineModel::scheduleTaskAt(const QString &taskId, int startMinutes, int snapMinutes)
{
    if (snapMinutes < 1)
        snapMinutes = 1;
    int snapped = ((startMinutes + snapMinutes / 2) / snapMinutes) * snapMinutes;
    snapped = std::clamp(snapped, 0, 24 * 60 - 1);

    const QDateTime localStart(m_date, QTime(snapped / 60, snapped % 60));
    const TaskNode *task = m_tree.findById(taskId);
    if (!task)
        return false;

    // Give it a visible length: its target if it has one, else an hour.
    const qint64 lengthMs = task->targetMs > 0 ? task->targetMs : 60LL * 60 * 1000;
    if (!m_tree.rescheduleTask(taskId, localStart.toUTC()))
        return false;
    if (TaskNode *mutableTask = m_tree.findById(taskId)) {
        if (!mutableTask->scheduledEndAt.isValid())
            mutableTask->scheduledEndAt = localStart.toUTC().addMSecs(lengthMs);
        m_tree.saveTask(taskId);
    }

    refresh();
    return true;
}

bool DayTimelineModel::unscheduleTask(const QString &taskId)
{
    TaskNode *task = m_tree.findById(taskId);
    if (!task)
        return false;
    task->scheduledStartAt = QDateTime();
    task->scheduledEndAt = QDateTime();
    if (!m_tree.saveTask(taskId))
        return false;
    refresh();
    return true;
}

int DayTimelineModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_blocks.size();
}

QVariant DayTimelineModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_blocks.size())
        return {};
    const Block &b = m_blocks.at(index.row());
    switch (role) {
    case TaskIdRole: return b.taskId;
    case TitleRole: return b.title;
    case KindRole: return b.kind;
    case StartMinutesRole: return b.startMinutes;
    case DurationMinutesRole: return b.durationMinutes;
    case LabelRole: return b.label;
    case OverTargetRole: return b.overTarget;
    case MovableRole: return b.movable;
    case LaneRole: return b.lane;
    case LaneCountRole: return b.laneCount;
    case Qt::DisplayRole: return b.label;
    default: return {};
    }
}

QHash<int, QByteArray> DayTimelineModel::roleNames() const
{
    return {
        {TaskIdRole, "taskId"},
        {TitleRole, "title"},
        {KindRole, "kind"},
        {StartMinutesRole, "startMinutes"},
        {DurationMinutesRole, "durationMinutes"},
        {LabelRole, "label"},
        {OverTargetRole, "overTarget"},
        {MovableRole, "movable"},
        {LaneRole, "lane"},
        {LaneCountRole, "laneCount"},
    };
}

void DayTimelineModel::refresh()
{
    beginResetModel();
    rebuild();
    endResetModel();
    emit contentChanged();
}

void DayTimelineModel::tick()
{
    emit nowChanged();
}

void DayTimelineModel::assignLanes()
{
    // Side-by-side columns for blocks that overlap in time. Without this a
    // planned block and the session actually logged against it sit on top of
    // each other and neither is readable. Expects m_blocks sorted by start.
    int groupStart = 0;
    int groupEnd = -1;       // latest end time seen in the current cluster
    QVector<int> laneEnds;   // end time per lane within the cluster

    auto closeGroup = [&](int endIndex) {
        const int lanes = std::max(1, static_cast<int>(laneEnds.size()));
        for (int i = groupStart; i < endIndex; ++i)
            m_blocks[i].laneCount = lanes;
        laneEnds.clear();
    };

    for (int i = 0; i < m_blocks.size(); ++i) {
        Block &b = m_blocks[i];
        const int end = b.startMinutes + b.durationMinutes;

        // A block starting at or after every end so far begins a fresh cluster.
        if (groupEnd >= 0 && b.startMinutes >= groupEnd) {
            closeGroup(i);
            groupStart = i;
            groupEnd = -1;
        }

        int lane = -1;
        for (int l = 0; l < static_cast<int>(laneEnds.size()); ++l) {
            if (laneEnds[l] <= b.startMinutes) {
                lane = l;
                break;
            }
        }
        if (lane < 0) {
            lane = static_cast<int>(laneEnds.size());
            laneEnds.push_back(end);
        } else {
            laneEnds[lane] = end;
        }
        b.lane = lane;
        groupEnd = groupEnd < 0 ? end : std::max(groupEnd, end);
    }
    closeGroup(static_cast<int>(m_blocks.size()));
}

void DayTimelineModel::rebuild()
{
    m_blocks.clear();
    m_loggedMsForDay = 0;

    const QDateTime dayStartLocal(m_date, QTime(0, 0));
    const QDateTime dayEndLocal = dayStartLocal.addDays(1);

    // Planned blocks: tasks whose scheduled start falls on this local day.
    for (const TaskNode *task : m_tree.allTasks()) {
        if (!task || !task->scheduledStartAt.isValid())
            continue;
        const QDateTime startLocal = task->scheduledStartAt.toLocalTime();
        if (startLocal.date() != m_date)
            continue;

        int durationMin = 60;
        if (task->scheduledEndAt.isValid()) {
            const qint64 ms = task->scheduledStartAt.msecsTo(task->scheduledEndAt);
            if (ms > 0)
                durationMin = static_cast<int>(ms / 60000);
        } else if (task->targetMs > 0) {
            durationMin = static_cast<int>(task->targetMs / 60000);
        }
        durationMin = std::max(durationMin, 15);

        Block b;
        b.taskId = task->id;
        b.title = task->title;
        b.kind = QStringLiteral("planned");
        b.startMinutes = minutesFromMidnight(startLocal);
        b.durationMinutes = durationMin;
        b.label = QStringLiteral("%1 planned · %2")
                      .arg(startLocal.toString(QStringLiteral("HH:mm")), task->title);
        b.movable = true;
        m_blocks.push_back(b);
    }

    // Logged blocks: real sessions recorded on this local day.
    for (const SessionRecord &s : m_tree.sessionsBetween(dayStartLocal.toUTC(), dayEndLocal.toUTC())) {
        const TaskNode *task = m_tree.findById(s.taskId);
        const QString title = task ? task->title : QStringLiteral("(deleted task)");
        const QDateTime startLocal = s.startedAt.toLocalTime();

        Block b;
        b.taskId = s.taskId;
        b.title = title;
        b.kind = QStringLiteral("logged");
        b.startMinutes = minutesFromMidnight(startLocal);
        b.durationMinutes = std::max(static_cast<int>(s.durationMs / 60000), 10);
        b.label = QStringLiteral("%1 · %2").arg(title, durationLabel(s.durationMs));
        b.overTarget = task && task->targetMs > 0
                       && m_tree.progressMs(*task, QStringLiteral("logged")) > task->targetMs;
        b.movable = false;
        m_blocks.push_back(b);
        m_loggedMsForDay += s.durationMs;
    }

    std::sort(m_blocks.begin(), m_blocks.end(), [](const Block &a, const Block &b) {
        return a.startMinutes < b.startMinutes;
    });

    assignLanes();

    // Fit the visible hour range to the day's content. 09:00-15:00 is only a
    // fallback for an empty day, never a floor: treating it as a floor pinned
    // the view to 09:00 and pushed an evening's blocks below the fold.
    int earliest = -1;
    int latest = -1;
    for (const Block &b : m_blocks) {
        const int end = b.startMinutes + b.durationMinutes;
        earliest = earliest < 0 ? b.startMinutes : std::min(earliest, b.startMinutes);
        latest = latest < 0 ? end : std::max(latest, end);
    }
    if (showingToday()) {
        const int now = nowMinutes();
        earliest = earliest < 0 ? now : std::min(earliest, now);
        latest = latest < 0 ? now + 60 : std::max(latest, now + 30);
    }
    if (earliest < 0) {
        earliest = 9 * 60;
        latest = 15 * 60;
    }

    earliest = std::max(0, earliest - 30);
    latest = std::min(24 * 60, latest + 30);

    m_firstHour = std::clamp(earliest / 60, 0, 23);
    const int lastHour = std::clamp((latest + 59) / 60, m_firstHour + 1, 24);
    m_hourCount = std::clamp(lastHour - m_firstHour, 4, 24 - m_firstHour);
}

} // namespace polomodoro
