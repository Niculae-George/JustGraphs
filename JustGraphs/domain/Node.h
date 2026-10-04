#ifndef NODE_H
#define NODE_H
#include <QtCore/QPoint>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <map>


class Node
{
    int m_index;
    QPoint m_coord;
    QString m_name;
    std::map<QString, QVariant> m_metadata;
public:
    Node(int index = 0, QPoint p = QPoint(0, 0), QString name = "");
    void setCoord(QPoint p);
    void setName(const QString& name);
    int getIndex() const;
    const QPoint& getCoord() const;
    QPoint getPos() const { return m_coord; }
    QString getName() const;
    int getX() const;
    int getY() const;

    // Metadata system
    void setProperty(const QString& key, const QVariant& value);
    QVariant getProperty(const QString& key, const QVariant& defaultValue = QVariant()) const;
    bool hasProperty(const QString& key) const;
};

#endif // NODE_H
