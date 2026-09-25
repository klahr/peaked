#ifndef MONOTONE_H
#define MONOTONE_H

#include <QDateTime>
#include <QVector>

// Good is 1, ok 0 and bad -1, averaged per bin of a measure and weighted
struct ScoreBin {
    int index;
    double weight;
    double score;
};

// The score per bin made non-increasing with the index, nobody feels better
// from more alcohol. Pool adjacent violators, weighted. Bins in index order.
QVector<ScoreBin> monotoneDecreasing(QVector<ScoreBin> bins);

// Older records count less, tolerance changes. Half after 60 days.
double recencyWeight(const QDateTime &time, const QDateTime &now);

#endif // MONOTONE_H
