#ifndef EDGE_H
#define EDGE_H
#include "domain/Node.h"

class Edge
{
    const Node *m_first, *m_second;
    int m_cost;
public:
    Edge(const Node* f, const Node* s, int cost = 1);
    const Node& getFirst() const;
    const Node& getSecond() const;
    int getCost() const;
    void setCost(int cost);
};


#endif // EDGE_H
