#include "maps/luxembourg_map.h"

#include <QXmlStreamReader>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <utility>

namespace Luxembourg {
namespace {

bool attribute(const QXmlStreamAttributes& attrs, const char* key, QString& value)
{
    const auto a = attrs.value(QLatin1String(key));
    if (a.isNull()) return false;
    value = a.toString();
    return true;
}

bool parseInt(const QString& text, int& value)
{
    bool ok = false;
    const qlonglong parsed = text.toLongLong(&ok);
    if (!ok || parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) return false;
    value = static_cast<int>(parsed);
    return true;
}

bool parseCoordinate(const QString& text, double& value)
{
    bool ok = false;
    value = text.toDouble(&ok);
    return ok && std::isfinite(value);
}

} // namespace

bool Map::load(QIODevice& input, QString& error)
{
    nodes_.clear();
    arcs_.clear();
    bounds_ = QRectF();
    adjacency_.clear();
    reverseAdjacency_.clear();
    error.clear();
    auto fail = [&](const QString& message) {
        nodes_.clear(); arcs_.clear(); adjacency_.clear(); reverseAdjacency_.clear(); bounds_ = QRectF();
        error = message;
        return false;
    };
    if (!input.isOpen() && !input.open(QIODevice::ReadOnly)) return fail(QStringLiteral("Unable to open map input."));

    QXmlStreamReader xml(&input);
    bool rootSeen = false, nodesSeen = false, arcsSeen = false;
    QString currentSection;
    int depth = 0;
    std::unordered_map<int, int> idToIndex;
    std::vector<double> rawLongitudes;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement()) {
            if (depth == 2 && (xml.name() == QLatin1String("nodes") || xml.name() == QLatin1String("arcs")))
                currentSection.clear();
            --depth;
            continue;
        }
        if (!xml.isStartElement()) continue;
        const QString name = xml.name().toString();
        if (!rootSeen) {
            if (name != QLatin1String("map")) return fail(QStringLiteral("Expected <map> root element."));
            rootSeen = true;
        } else if (name == QLatin1String("nodes")) {
            if (depth != 1 || nodesSeen || arcsSeen) return fail(QStringLiteral("Unexpected or duplicate <nodes> element."));
            nodesSeen = true;
            currentSection = name;
        } else if (name == QLatin1String("node")) {
            if (depth != 2 || currentSection != QLatin1String("nodes")) return fail(QStringLiteral("<node> must appear inside <nodes>."));
            QString idText, longitudeText, latitudeText;
            int id; double longitude, latitude;
            const auto attrs = xml.attributes();
            if (!attribute(attrs, "id", idText) || !attribute(attrs, "longitude", longitudeText) || !attribute(attrs, "latitude", latitudeText)
                || !parseInt(idText, id) || !parseCoordinate(longitudeText, longitude) || !parseCoordinate(latitudeText, latitude))
                return fail(QStringLiteral("Malformed node id or coordinate at line %1.").arg(xml.lineNumber()));
            if (idToIndex.find(id) != idToIndex.end()) return fail(QStringLiteral("Duplicate node id %1.").arg(id));
            idToIndex.emplace(id, static_cast<int>(nodes_.size()));
            // This dataset names coordinates backwards: longitude is north/south
            // (~49.6 * 100000), latitude is east/west (~6.2 * 100000).
            nodes_.push_back({id, QPointF(latitude, -longitude)});
            rawLongitudes.push_back(longitude);
        } else if (name == QLatin1String("arcs")) {
            if (depth != 1 || !nodesSeen || arcsSeen) return fail(QStringLiteral("Unexpected or duplicate <arcs> element."));
            arcsSeen = true;
            currentSection = name;
        } else if (name == QLatin1String("arc")) {
            if (depth != 2 || currentSection != QLatin1String("arcs")) return fail(QStringLiteral("<arc> must appear inside <arcs>."));
            QString fromText, toText, lengthText;
            int from, to;
            bool ok = false;
            const auto attrs = xml.attributes();
            const qlonglong length = (attribute(attrs, "length", lengthText) ? lengthText.toLongLong(&ok) : 0);
            if (!attribute(attrs, "from", fromText) || !attribute(attrs, "to", toText)
                || !parseInt(fromText, from) || !parseInt(toText, to) || !ok || length < 0)
                return fail(QStringLiteral("Malformed or negative arc at line %1.").arg(xml.lineNumber()));
            const auto fromIt = idToIndex.find(from);
            const auto toIt = idToIndex.find(to);
            if (fromIt == idToIndex.end() || toIt == idToIndex.end())
                return fail(QStringLiteral("Arc endpoint missing at line %1.").arg(xml.lineNumber()));
            arcs_.push_back({fromIt->second, toIt->second, static_cast<qint64>(length)});
        } else {
            return fail(QStringLiteral("Unexpected element <%1> at line %2.").arg(name).arg(xml.lineNumber()));
        }
        ++depth;
    }
    if (xml.hasError()) return fail(QStringLiteral("XML parse error at line %1: %2").arg(xml.lineNumber()).arg(xml.errorString()));
    if (!rootSeen || !nodesSeen || !arcsSeen || nodes_.empty()) return fail(QStringLiteral("Map must contain nonempty <nodes> and <arcs> sections."));

    // Scale the actual east/west axis at the mean geographic latitude.
    double centerLongitude = 0.0;
    for (double longitude : rawLongitudes) centerLongitude += longitude / static_cast<double>(rawLongitudes.size());
    constexpr double pi = 3.14159265358979323846;
    const double longitudeScale = std::cos(centerLongitude / 100000.0 * pi / 180.0);
    double minX = std::numeric_limits<double>::infinity(), maxX = -minX;
    double minY = minX, maxY = -minX;
    for (size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i].position.setX(nodes_[i].position.x() * longitudeScale);
        const QPointF p = nodes_[i].position;
        minX = std::min(minX, p.x()); maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y()); maxY = std::max(maxY, p.y());
    }
    bounds_ = QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized();

    adjacency_.resize(nodes_.size());
    reverseAdjacency_.resize(nodes_.size());
    for (const RoadArc& arc : arcs_) {
        adjacency_[static_cast<size_t>(arc.from)].emplace_back(arc.to, arc.length);
        reverseAdjacency_[static_cast<size_t>(arc.to)].emplace_back(arc.from, arc.length);
    }
    return true;
}

int Map::nearestNode(QPointF worldPosition) const
{
    int nearest = -1;
    double best = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < nodes_.size(); ++i) {
        const double dx = nodes_[i].position.x() - worldPosition.x();
        const double dy = nodes_[i].position.y() - worldPosition.y();
        const double distance = dx * dx + dy * dy;
        if (distance < best) { best = distance; nearest = static_cast<int>(i); }
    }
    return nearest;
}

} // namespace Luxembourg
