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
    return QStringLiteral("%1 ACTIVE · %2 LOGGED")
        .arg(m_activeCount)
        .arg(m_loggedMsForDay > 0 ? durationLabel(m_loggedMsForDay) : QStringLiteral("0m"))
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
int DayTimelineModel::scrollToMinutes() const { return m_scrollToMinutes; }

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
    case CollapsedRole: return b.collapsed;
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
        {CollapsedRole, "collapsed"},
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
    // A running block grows with the clock, so it needs a rebuild, not just a
    // repositioned marker. Spec has both share one 60 s timer.
    bool hasRunning = false;
    for (const Block &b : m_blocks) {
        if (b.kind == QLatin1String("running")) {
            hasRunning = true;
            break;
        }
    }
    if (hasRunning)
        refresh();
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

    // Spec caps the split at 3 lanes; anything beyond collapses to a tick
    // rather than shrinking every column into illegibility.
    static constexpr int kMaxLanes = 3;
    // A block is never drawn shorter than 18 px, which at 72 px/hour is 15
    // minutes. Lanes must be assigned against that rendered extent, or a
    // 3-minute session ends up painted under its neighbour and unclickable.
    static constexpr int kMinRenderedMinutes = 15;
    auto renderedEnd = [](const Block &b) {
        return b.startMinutes + std::max(b.durationMinutes, kMinRenderedMinutes);
    };

    auto closeGroup = [&](int endIndex) {
        const int lanes = std::clamp(static_cast<int>(laneEnds.size()), 1, kMaxLanes);
        for (int i = groupStart; i < endIndex; ++i)
            m_blocks[i].laneCount = lanes;
        laneEnds.clear();
    };

    for (int i = 0; i < m_blocks.size(); ++i) {
        Block &b = m_blocks[i];
        const int end = renderedEnd(b);

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
        // Keep the true lane index even past the cap: the view uses it to fan
        // the collapsed ticks out instead of piling them on the last column.
        b.lane = lane;
        b.collapsed = lane >= kMaxLanes;
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

    // Logged blocks: real sessions recorded on this local day. Sessions of the
    // same task less than 2 minutes apart are merged, so a pomodoro cycle reads
    // as one bar rather than a stack of near-touching slivers.
    struct Merged {
        QString taskId;
        QDateTime startLocal;
        qint64 durationMs = 0;
    };
    QVector<Merged> merged;
    for (const SessionRecord &s : m_tree.sessionsBetween(dayStartLocal.toUTC(), dayEndLocal.toUTC())) {
        const QDateTime startLocal = s.startedAt.toLocalTime();
        bool joined = false;
        for (Merged &m : merged) {
            if (m.taskId != s.taskId)
                continue;
            const QDateTime mEnd = m.startLocal.addMSecs(m.durationMs);
            const qint64 gapMs = mEnd.msecsTo(startLocal);
            if (gapMs >= 0 && gapMs < 2 * 60 * 1000) {
                m.durationMs = m.startLocal.msecsTo(startLocal) + s.durationMs;
                joined = true;
                break;
            }
        }
        if (!joined)
            merged.push_back({s.taskId, startLocal, s.durationMs});
        m_loggedMsForDay += s.durationMs;
    }

    for (const Merged &m : merged) {
        const TaskNode *task = m_tree.findById(m.taskId);
        const QString title = task ? task->title : QStringLiteral("(deleted task)");

        Block b;
        b.taskId = m.taskId;
        b.title = title;
        b.kind = QStringLiteral("logged");
        b.startMinutes = minutesFromMidnight(m.startLocal);
        b.durationMinutes = static_cast<int>(m.durationMs / 60000);
        b.label = QStringLiteral("%1 · %2").arg(title, durationLabel(m.durationMs));
        b.overTarget = task && task->targetMs > 0
                       && m_tree.progressMs(*task, QStringLiteral("logged")) > task->targetMs;
        b.movable = false;
        m_blocks.push_back(b);
    }

    // In-progress blocks: an active task's current segment, open-ended and
    // growing with the clock. Only meaningful on the day it started.
    m_activeCount = 0;
    for (const TaskNode *task : m_tree.allTasks()) {
        if (!task || task->status != TaskStatus::Active)
            continue;
        ++m_activeCount;
        if (!task->activeSince.isValid())
            continue;
        const QDateTime startLocal = task->activeSince.toLocalTime();
        if (startLocal.date() != m_date)
            continue;

        const qint64 runningMs = startLocal.msecsTo(QDateTime::currentDateTime());
        Block b;
        b.taskId = task->id;
        b.title = task->title;
        b.kind = QStringLiteral("running");
        b.startMinutes = minutesFromMidnight(startLocal);
        b.durationMinutes = static_cast<int>(std::max<qint64>(runningMs, 0) / 60000);
        b.label = QStringLiteral("%1 · %2").arg(task->title, durationLabel(std::max<qint64>(runningMs, 0)));
        b.overTarget = task->targetMs > 0
                       && m_tree.progressMs(*task, QStringLiteral("active")) > task->targetMs;
        b.movable = false;
        m_blocks.push_back(b);
    }

    std::sort(m_blocks.begin(), m_blocks.end(), [](const Block &a, const Block &b) {
        return a.startMinutes < b.startMinutes;
    });

    assignLanes();

    // The column is a fixed 24 h, so the only decision is where it opens:
    // the now line on today, otherwise an hour before the day's first block.
    if (showingToday()) {
        m_scrollToMinutes = nowMinutes();
    } else {
        int earliest = -1;
        for (const Block &b : m_blocks)
            earliest = earliest < 0 ? b.startMinutes : std::min(earliest, b.startMinutes);
        m_scrollToMinutes = earliest < 0 ? 9 * 60 : std::max(0, earliest - 60);
    }
}

} // namespace polomodoro
