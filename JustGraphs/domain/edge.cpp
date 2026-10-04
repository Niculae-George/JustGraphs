#include "domain/edge.h"
#include <stdexcept>

Edge::Edge(const Node* f, const Node* s, int cost)
    : m_first(f), m_second(s), m_cost(cost) {
    if (!f || !s) throw std::invalid_argument("edge endpoints must not be null");
}

const Node& Edge::getFirst() const { return *m_first; }
const Node& Edge::getSecond() const { return *m_second; }

int Edge::getCost() const { return m_cost; }
void Edge::setCost(int cost) { m_cost = cost; }
