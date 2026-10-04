#include "domain/Node.h"

Node::Node(int index, QPoint p, QString name) : m_index(index), m_coord(p), m_name(name)
{
}

void Node::setCoord(QPoint p) {
    m_coord = p;
}

void Node::setName(const QString& name) {
    m_name = name;
}

int Node::getIndex() const {
    return m_index;
}

const QPoint& Node::getCoord() const
{
    return m_coord;
}

QString Node::getName() const {
    return m_name;
}

int Node::getX() const {
    return m_coord.x();
}

int Node::getY() const {
    return m_coord.y();
}

void Node::setProperty(const QString& key, const QVariant& value) {
    m_metadata[key] = value;
}

QVariant Node::getProperty(const QString& key, const QVariant& defaultValue) const {
    auto it = m_metadata.find(key);
    if (it != m_metadata.end()) return it->second;
    return defaultValue;
}

bool Node::hasProperty(const QString& key) const {
    return m_metadata.find(key) != m_metadata.end();
}
