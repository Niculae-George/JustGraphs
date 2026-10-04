#pragma once
#include <QString>
#include <vector>

class QWidget;

struct PageDescriptor {
    QString id;
    QString navigationLabel;
    QString title;
    QString subtitle;
    QString keywords;
    QWidget* (*create)(QWidget* parent);
};

// One composition list supplies page construction, labels, cards, and search.
const std::vector<PageDescriptor>& pageDescriptors();
