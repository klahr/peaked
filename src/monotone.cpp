#include "monotone.h"

#include <QtMath>

static const double HalfLifeDays = 60.0;

QVector<ScoreBin> monotoneDecreasing(QVector<ScoreBin> bins)
{
    struct Block {
        int first;
        int last;
        double weight;
        double sum;
    };
    QVector<Block> blocks;
    for (int i = 0; i < bins.size(); ++i) {
        blocks.append({ i, i, bins.at(i).weight, bins.at(i).score * bins.at(i).weight });
        while (blocks.size() > 1) {
            const Block &last = blocks.at(blocks.size() - 1);
            const Block &previous = blocks.at(blocks.size() - 2);
            if (previous.sum / previous.weight >= last.sum / last.weight)
                break;
            const Block merged { previous.first, last.last, previous.weight + last.weight, previous.sum + last.sum };
            blocks.removeLast();
            blocks.last() = merged;
        }
    }
    for (const Block &block : blocks) {
        for (int i = block.first; i <= block.last; ++i)
            bins[i].score = block.sum / block.weight;
    }
    return bins;
}

double recencyWeight(const QDateTime &time, const QDateTime &now)
{
    return qPow(0.5, time.msecsTo(now) / 86400000.0 / HalfLifeDays);
}
