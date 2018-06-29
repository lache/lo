#include "precompiled.hpp"
#include "astarrtree.hpp"
#include "AStar.h"
#include "CohenSutherland.h"

using namespace astarrtree;

struct PathfindContext {
    xy32xy32 from_rect; // 1x1 rect version of from-point
    xy32xy32 to_rect; // 1x1 rect version of to-point
    xy32xy32 from_box; // cell which contains from-point
    xy32xy32 to_box; // cell which contains to-point
    const rtree* rtree_ptr;
    std::shared_ptr<coro_context> coro;
    int early_exit_call_count;
    std::unordered_map<void*, std::pair<size_t, size_t> > neighbor_cache_map;
    std::vector<std::pair<xy32xy32xy32, float> > neighbor_cache;
};

xy32xy32 astarrtree::xyxy_from_box_t(const box& v) {
    xy32xy32 r;
    r.xy0.x = v.min_corner().get<0>();
    r.xy0.y = v.min_corner().get<1>();
    r.xy1.x = v.max_corner().get<0>();
    r.xy1.y = v.max_corner().get<1>();
    return r;
}

box astarrtree::box_t_from_xyxy(const xy32xy32& v) {
    box r;
    r.min_corner().set<0>(v.xy0.x);
    r.min_corner().set<1>(v.xy0.y);
    r.max_corner().set<0>(v.xy1.x);
    r.max_corner().set<1>(v.xy1.y);
    return r;
}

double astarrtree::xy32_distance(const xy32& a, const xy32& b) {
    const long long dx = b.x - a.x;
    const long long dy = b.y - a.y;
    return sqrt(static_cast<double>(dx*dx + dy * dy));
}

static int xy32_comparator(const xy32& p1, const xy32& p2) {
    const auto v1 = (static_cast<int64_t>(p1.y) << 32) | static_cast<int64_t>(p1.x);
    const auto v2 = (static_cast<int64_t>(p2.y) << 32) | static_cast<int64_t>(p2.x);
    const auto d = v1 - v2;
    if (d == 0) {
        return 0;
    } else if (d > 0) {
        return 1;
    } else {
        return -1;
    }
}

void AddNeighborWithLog(ASNeighborList neighbors, const xy32xy32xy32* n, xy32xy32xy32* n2, float edge_cost) {
    ASNeighborListAdd(neighbors, n2, edge_cost);
    LOGIx("Cell (%||,%||)-(%||,%||)[%||x%||] Point (%||,%||) neighbor Cell (%||,%||)-(%||,%||)[%||x%||] Point (%||,%||) added. Edge cost = %||",
          n->box.xy0.x,
          n->box.xy0.y,
          n->box.xy1.x,
          n->box.xy1.y,
          n->box.xy1.x - n->box.xy0.x,
          n->box.xy1.y - n->box.xy0.y,
          n->point.x,
          n->point.y,
          n2->box.xy0.x,
          n2->box.xy0.y,
          n2->box.xy1.x,
          n2->box.xy1.y,
          n2->box.xy1.x - n2->box.xy0.x,
          n2->box.xy1.y - n2->box.xy0.y,
          n2->point.x,
          n2->point.y,
          edge_cost);

}

void RTreePathNodeNeighbors(ASNeighborList neighbors, void *node, void *context) {
    auto pathfind_context = reinterpret_cast<PathfindContext*>(context);
    auto n = reinterpret_cast<const xy32xy32xy32*>(node);
    auto cache_it = pathfind_context->neighbor_cache_map.find(node);
    if (cache_it != pathfind_context->neighbor_cache_map.end()) {
        // neighbor cache hit
        for (size_t i = cache_it->second.first; i < cache_it->second.second; i++) {
            auto& nc = pathfind_context->neighbor_cache[i];
            AddNeighborWithLog(neighbors, n, &nc.first, nc.second);
        }
    } else {
        // neighbor cache no hit
        const rtree* rtree_ptr = pathfind_context->rtree_ptr;
        box query_box = box_t_from_xyxy(n->box);
        std::vector<value> result_s;
        rtree_ptr->query(bgi::intersects(query_box), std::back_inserter(result_s));
        const size_t begin_index = pathfind_context->neighbor_cache.size();
        double npx_clipped, npy_clipped, closest_to_goal_x, closest_to_goal_y;
        int accept = CohenSutherlandLineClip(static_cast<double>(n->box.xy0.x),
                                             static_cast<double>(n->box.xy0.y),
                                             static_cast<double>(n->box.xy1.x),
                                             static_cast<double>(n->box.xy1.y),
                                             static_cast<double>(n->point.x) + 0.5,
                                             static_cast<double>(n->point.y) + 0.5,
                                             static_cast<double>(pathfind_context->to_rect.xy0.x) + 0.5,
                                             static_cast<double>(pathfind_context->to_rect.xy0.y) + 0.5,
                                             &npx_clipped,
                                             &npy_clipped,
                                             &closest_to_goal_x,
                                             &closest_to_goal_y);
        if (accept == 0) {
            LOGE("Not clipped");
            abort();
        }
        for (const auto& v : result_s) {
            // skip 'node' itself from neighbor
            if (query_box.min_corner().get<0>() == v.first.min_corner().get<0>()
                && query_box.min_corner().get<1>() == v.first.min_corner().get<1>()) {
                continue;
            }
            auto xyxy = xyxy_from_box_t(v.first);
            const xy32 enter_point{
                boost::algorithm::clamp(static_cast<int>(closest_to_goal_x), xyxy.xy0.x, xyxy.xy1.x - 1),
                boost::algorithm::clamp(static_cast<int>(closest_to_goal_y), xyxy.xy0.y, xyxy.xy1.y - 1),
            };
            xy32xy32xy32 n2 = { xyxy, enter_point };
            const long long dx = enter_point.x - n->point.x;
            const long long dy = enter_point.y - n->point.y;
            const double edge_cost_cell_to_cell = sqrt(static_cast<double>(dx * dx + dy * dy));
            double edge_cost_cell_to_goal = 0;
            // if this neighbor('v') is goal node, we should add edge_cost_cell_to_goal factor
            if (xy32_comparator(xyxy.xy0, pathfind_context->to_box.xy0) == 0) {
                edge_cost_cell_to_goal = xy32_distance(enter_point, pathfind_context->to_rect.xy0);
            }
            const double edge_cost = edge_cost_cell_to_cell + edge_cost_cell_to_goal;
            AddNeighborWithLog(neighbors, n, &n2, static_cast<float>(edge_cost));
            pathfind_context->neighbor_cache.push_back(std::make_pair(n2, static_cast<float>(edge_cost)));
        }
        const size_t end_index = pathfind_context->neighbor_cache.size();
        pathfind_context->neighbor_cache_map[node] = std::make_pair(begin_index, end_index);
    }

}

int RectDistance(const xy32xy32* a, const xy32xy32* b) {
    int dx = 0, dy = 0;
    // check overlapping along x axis
    const int a_and_b_x = a->xy1.x - b->xy0.x;
    const int b_and_a_x = b->xy1.x - a->xy0.x;
    if (a_and_b_x < 0) {
        dx = -a_and_b_x;
    } else if (b_and_a_x < 0) {
        dx = -b_and_a_x;
    }
    // check overlapping along y axis
    const int a_and_b_y = a->xy1.y - b->xy0.y;
    const int b_and_a_y = b->xy1.y - a->xy0.y;
    if (a_and_b_y < 0) {
        dy = -a_and_b_y;
    } else if (b_and_a_y < 0) {
        dy = -b_and_a_y;
    }
    assert(dx >= 0);
    assert(dy >= 0);
    return dx + dy;
}

// https://www.sanfoundry.com/cpp-program-implement-gift-wrapping-algorithm-two-dimensions/

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int orientation(ixy32 p, ixy32 q, ixy32 r) {
    int val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

    if (val == 0)
        return 0; // colinear
    return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// Prints convex hull of a set of n points.
void convexHull(ixy32 points[], size_t n, float& shortest_len) {
    LOGI("Convex Hull calculation with %1% points...", n);
    // There must be at least 3 points
    if (n < 3) {
        return;
    }


    // Initialize Result
    int* next = reinterpret_cast<int*>(alloca(sizeof(int) * n));
    for (int i = 0; i < n; i++) {
        next[i] = -1;
    }


    // Find the leftmost point
    int l = 0;
    for (int i = 1; i < n; i++) {
        if (points[i].x < points[l].x) {
            l = i;
        }
    }


    // Start from leftmost point, keep moving counterclockwise
    // until reach the start point again
    int p = l, q;
    do {
        q = (p + 1) % n;
        // Search for a point 'q' such that orientation(p, i, q) is
        // counterclockwise for all points 'i'
        for (int i = 0; i < n; i++) {
            if (orientation(points[p], points[i], points[q]) == 2) {
                q = i;
                //i = -1;
            }
        }

        next[p] = q; // Add q to result as a next point of p
        p = q; // Set p as q for next iteration
    } while (p != l);

    // Print Result

    p = l;
    bool from = false;
    bool to = false;
    float total_circum = 0;
    bool from_to_segment = false;
    float segment_circum = 0;
    do {
        if (points[p].i == 0) {
            from = true;
            from_to_segment = !from_to_segment;
        }
        if (points[p].i == 2) {
            to = true;
            from_to_segment = !from_to_segment;
        }
        LOGIx("POINT %1%,%2% [INDEX=%3%]", points[p].x, points[p].y, points[p].i);

        const auto dx = static_cast<float>(points[next[p]].x - points[p].x);
        const auto dy = static_cast<float>(points[next[p]].y - points[p].y);
        const auto dlen = sqrtf(dx * dx + dy * dy);
        total_circum += dlen;
        if (std::isnan(total_circum)) {
            int x = 1000;
        }
        if (from_to_segment) {
            segment_circum += dlen;
        }

        p = next[p];
    } while (p != l);

    if (from && to) {
        shortest_len = std::min(segment_circum, total_circum - segment_circum);
        LOGI("GOOD! Total Circum %1%, Segment Circum %2%, Shortest %3%", total_circum, segment_circum, shortest_len);
    } else {
        shortest_len = total_circum;
    }
}

typedef std::pair<point, int> point_value_t;
const float pi = boost::math::constants::pi<float>();

bool check_not_duplicate(const xy32& from, const xy32& to, const ixy32& p) {
    return (from.x != p.x || from.y != p.y) && (to.x != p.x || to.y != p.y);
}

float RTreePathNodeHeuristic(void *fromNode, void *toNode, void *context) {
    const auto from = reinterpret_cast<const xy32xy32xy32*>(fromNode);
    const auto to = reinterpret_cast<const xy32xy32xy32*>(toNode);

    //const int d1 = from->box.xy1.x <= to->point.x ? (1 + to->point.x - from->box.xy1.x) : from->box.xy0.x > to->point.x ? (from->box.xy0.x - to->point.x) : 0;
    //const int d2 = from->box.xy1.y <= to->point.y ? (1 + to->point.y - from->box.xy1.y) : from->box.xy0.y > to->point.y ? (from->box.xy0.y - to->point.y) : 0;
    //return sqrtf((float)d1 * d1 + d2 * d2);

    return static_cast<float>(xy32_distance(from->point, to->point));


    // [1] Cell Midpoint Distance Method
    // TOO SLOW; illogical on very big cells
    const auto fromMedX = (from->box.xy1.y - from->box.xy0.x) / 2.0f;
    const auto fromMedY = (from->box.xy1.y - from->box.xy0.y) / 2.0f;
    const auto toMedX = (to->box.xy1.x - to->box.xy0.x) / 2.0f;
    const auto toMedY = (to->box.xy1.y - to->box.xy0.y) / 2.0f;
    /*
    return fabsf(fromMedX - toMedX) + fabsf(fromMedY - toMedY);*/

    // [2] Global Direction Following Method
    // Less cost if moving to the same direction... (kind of greedy)
    const auto pathfind_context = reinterpret_cast<PathfindContext*>(context);
    const auto global_from_rect = &pathfind_context->from_rect;
    const auto global_to_rect = &pathfind_context->to_rect;

    const auto dir_rect = atan2f(static_cast<float>(global_to_rect->xy0.y - global_from_rect->xy0.y),
                                 static_cast<float>(global_to_rect->xy0.x - global_from_rect->xy0.x));
    /*const float dir = atan2f(static_cast<float>(to->xy0.y - from->xy0.y),
                             static_cast<float>(to->xy0.x - from->xy0.x));*/
    const auto dir = atan2f(static_cast<float>(toMedY - fromMedY),
                            static_cast<float>(toMedX - fromMedX));
    //return fabsf(dir_rect - dir);

    // [3] Simple Manhattan Distance Method
    // INCORRECT on long cells
    //return static_cast<float>(abs(from->xy0.x - to->xy0.x) + abs(from->xy0.y - to->xy0.y));

    // [4] Rect Distance Method
    //return static_cast<float>(RectDistance(from, to) + RectDistance(to, to_rect));

    // [5] Rect Distance Method (Revised)
    const auto from_cost = RectDistance(&from->box, global_to_rect);
    const auto to_cost = RectDistance(&to->box, global_to_rect);
    const auto from_to_cost = RectDistance(&from->box, &to->box);
    //return static_cast<float>(from_to_cost != 0 ? FLT_MAX : to_cost);

    // [6] Enter Point Distance Method
    const auto enter_point_dx = from->point.x - to->point.x;
    const auto enter_point_dy = from->point.y - to->point.y;
    const auto enter_point_dist = sqrtf(static_cast<float>(enter_point_dx * enter_point_dx + enter_point_dy * enter_point_dy));
    float path_cost = 0;

    //if (from_to_cost == 0) {
    //    path_cost = enter_point_dist;
    //} else {
    //    // not neighbor
    //    // finding shortest path cost (distance) between 'from->point' and 'to->point'
    //    // with a rectangular obstacle exists on the straight path.
    //    int obstacle_min_corner_x = INT_MAX;
    //    int obstacle_min_corner_y = INT_MAX;
    //    int obstacle_max_corner_x = INT_MIN;
    //    int obstacle_max_corner_y = INT_MIN;

    //    bool seg_valid = false;
    //    bgm::segment<point> segment_between_points{ { from->point.x, from->point.y },{ to->point.x, to->point.y } };
    //    point from_point(from->point.x, from->point.y);
    //    point to_point(to->point.x, to->point.y);
    //    //bool seg_contained = false;

    //    std::vector<ixy32> A{
    //        { 0, from->point.x, from->point.y },
    //        { 2, to->point.x, to->point.y },
    //    };
    //    assert(from->point.x != to->point.x || from->point.y != to->point.y);

    //    for (auto seg_it = pathfind_context->rtree_land->qbegin(bgi::intersects(segment_between_points)); seg_it != pathfind_context->rtree_land->qend(); seg_it++) {
    //        if (boost::geometry::within(from_point, seg_it->first) || boost::geometry::within(to_point, seg_it->first)) {
    //            continue;
    //        }
    //        if (obstacle_min_corner_x > seg_it->first.min_corner().get<0>()) {
    //            obstacle_min_corner_x = seg_it->first.min_corner().get<0>();
    //        }
    //        if (obstacle_min_corner_y > seg_it->first.min_corner().get<1>()) {
    //            obstacle_min_corner_y = seg_it->first.min_corner().get<1>();
    //        }
    //        if (obstacle_max_corner_x < seg_it->first.max_corner().get<0>()) {
    //            obstacle_max_corner_x = seg_it->first.max_corner().get<0>();
    //        }
    //        if (obstacle_max_corner_y < seg_it->first.max_corner().get<1>()) {
    //            obstacle_max_corner_y = seg_it->first.max_corner().get<1>();
    //        }
    //        box obstacle{ { obstacle_min_corner_x, obstacle_min_corner_y },{ obstacle_max_corner_x ,obstacle_max_corner_y } };
    //        if (boost::geometry::within(from_point, obstacle)) {
    //            LOGEx("%1%: from point included in obstacle! from point: %2% %3% / obs: %4% %5% %6% %7%",
    //                  __func__,
    //                  from_point.get<0>(),
    //                  from_point.get<1>(),
    //                  obstacle.min_corner().get<0>(),
    //                  obstacle.min_corner().get<1>(),
    //                  obstacle.max_corner().get<0>(),
    //                  obstacle.max_corner().get<1>());
    //            //abort();
    //            //seg_contained = true;
    //        }
    //        if (boost::geometry::within(to_point, obstacle)) {
    //            LOGEx("%1%: to point included in obstacle! from point: %2% %3% / obs: %4% %5% %6% %7%",
    //                  __func__,
    //                  to_point.get<0>(),
    //                  to_point.get<1>(),
    //                  obstacle.min_corner().get<0>(),
    //                  obstacle.min_corner().get<1>(),
    //                  obstacle.max_corner().get<0>(),
    //                  obstacle.max_corner().get<1>());
    //            //abort();
    //            //seg_contained = true;
    //        }
    //        ixy32 p0{ 1, seg_it->first.min_corner().get<0>(), seg_it->first.min_corner().get<1>() };
    //        ixy32 p1{ 1, seg_it->first.min_corner().get<0>(), seg_it->first.max_corner().get<1>() };
    //        ixy32 p2{ 1, seg_it->first.max_corner().get<0>(), seg_it->first.min_corner().get<1>() };
    //        ixy32 p3{ 1, seg_it->first.max_corner().get<0>(), seg_it->first.max_corner().get<1>() };

    //        if ((from->point.x == to->point.x) && (from->point.x == seg_it->first.min_corner().get<0>() || from->point.x == seg_it->first.max_corner().get<0>())) {
    //            // COLINEAR ON Y-AXIS
    //        } else if ((from->point.y == to->point.y) && (from->point.y == seg_it->first.min_corner().get<1>() || from->point.y == seg_it->first.max_corner().get<1>())) {
    //            // COLINEAR ON X-AXIS
    //        } else {
    //            if (check_not_duplicate(from->point, to->point, p0)) {
    //                A.push_back(p0);
    //            }
    //            if (check_not_duplicate(from->point, to->point, p1)) {
    //                A.push_back(p1);
    //            }
    //            if (check_not_duplicate(from->point, to->point, p2)) {
    //                A.push_back(p2);
    //            }
    //            if (check_not_duplicate(from->point, to->point, p3)) {
    //                A.push_back(p3);
    //            }
    //            seg_valid = true;
    //            LOGI("Convex Hull node size %1%...", A.size());
    //        }
    //    }

    //    if (seg_valid) {
    //        convexHull(&A[0], A.size(), path_cost);
    //    } else {
    //        // yey! no obstacle!
    //        path_cost = enter_point_dist;
    //    }
    //}

    // 'from_to_cast' is a pentalty cost.
    // It is non-zero only if 'fromNode' and 'toNode' are not neighbors.
    return enter_point_dist + from_to_cost;
    //return path_cost;



    // [7] No cost
    //return 0;
}

int RTreePathNodeComparator(void *node1, void *node2, void *context) {
    const auto pathfind_context = reinterpret_cast<PathfindContext*>(context);
    const auto n1 = reinterpret_cast<const xy32xy32xy32*>(node1);
    const auto n2 = reinterpret_cast<const xy32xy32xy32*>(node2);
    // compare 'box.xy0' and then 'point'
    const auto box_xy0_compare_result = xy32_comparator(n1->box.xy0, n2->box.xy0);
    if (box_xy0_compare_result != 0) {
        return box_xy0_compare_result;
    } else {
        // n1->box === n2->box case:
        // if node1 and node2 have the same 'box.xy0' -- which means that they are the same cell
        // check 'point' if they are not 'goal' node ('to' rect)
        if (xy32_comparator(n1->box.xy0, pathfind_context->to_box.xy0) != 0) {
            return xy32_comparator(n1->point, n2->point);
        } else {
            return 0;
        }
    }
}

float xyib_distance(const xy32ibb& a, const xy32ibb& b) {
    const long long dx = a.p.x - b.p.x;
    const long long dy = a.p.y - b.p.y;
    return static_cast<float>(sqrt(static_cast<double>(dx * dx + dy * dy)));
}

struct pixel_waypoint_search {
    xy32 from;
    xy32 to;
    ASPath cell_path;
    std::shared_ptr<coro_context> coro;
    int early_exit_call_count;
};

void RTreePixelPathNodeNeighbors(ASNeighborList neighbors, void *node, void *context);
typedef void(*AStarNodeNeighborsCallback)(ASNeighborList neighbors, void *node, void *context);

void AddPixelNeighborWithLog(ASNeighborList neighbors, xy32ibb* n, pixel_waypoint_search* pws, int x, int y, size_t i, XYIBB_ENTER_EXIT ee, int nc, AStarNodeNeighborsCallback cb) {
    xy32ibb neighbor = { { x, y }, i, ee, nc };
    if (n->p.x == x && n->p.y == y && n->ee == XEE_ENTER && ee == XEE_EXIT && n->i == i) {
        // if 'n' and 'neighbor' have the same coordinates...
        // make 'n' exit node and rerun RTreePixelPathNodeNeighbors()
        n->ee = XEE_EXIT;
        cb(neighbors, n, pws);
    } else {
        ASNeighborListAdd(neighbors, &neighbor, xyib_distance(*n, neighbor));
        LOGIx("Neighbor of (%5d,%5d)[%5d] : (%5d,%5d)[%5d]",
              n->p.x,
              n->p.y,
              n->i,
              x,
              y,
              i);
    }
}

enum RECT_RELATION {
    RR_DOWN_RIGHT,
    RR_UP_RIGHT,
    RR_UP_LEFT,
    RR_DOWN_LEFT,
    RR_DOWN,
    RR_UP,
    RR_RIGHT,
    RR_LEFT,
    RR_UNKNOWN,
};


RECT_RELATION rect_relation(const xy32xy32* n1c, const xy32xy32* n2c) {
    bool d = false, u = false, r = false, l = false;
    if (n1c->xy1.y <= n2c->xy0.y) {
        // [D]OWN
        //  n1c
        // -----
        //  n2c
        d = true;
    }
    if (n2c->xy1.y <= n1c->xy0.y) {
        // [U]P
        //  n2c
        // -----
        //  n1c
        u = true;
    }
    if (n1c->xy1.x <= n2c->xy0.x) {
        // [R]IGHT
        //      |
        //  n1c | n2c
        //      |
        r = true;
    }
    if (n2c->xy1.x <= n1c->xy0.x) {
        // [L]EFT
        //      |
        //  n2c | n1c
        //      |
        l = true;
    }
    if (d && !u && r && !l) {
        return RR_DOWN_RIGHT;
    } else if (!d && u && r && !l) {
        return RR_UP_RIGHT;
    } else if (!d && u && !r && l) {
        return RR_UP_LEFT;
    } else if (d && !u && !r && l) {
        return RR_DOWN_LEFT;
    } else if (d && !u && !r && !l) {
        return RR_DOWN;
    } else if (!d && u && !r && !l) {
        return RR_UP;
    } else if (!d && !u && r && !l) {
        return RR_RIGHT;
    } else if (!d && !u && !r && l) {
        return RR_LEFT;
    } else {
        LOGE("Unknown rect relation...");
        abort();
        return RR_UNKNOWN;
    }
}

RECT_RELATION rect_neighbor_relation(const xy32xy32* n1c, const xy32xy32* n2c) {
    bool d = false, u = false, r = false, l = false;
    if (n1c->xy1.y == n2c->xy0.y) {
        // [D]OWN
        //  n1c
        // -----
        //  n2c
        d = true;
    }
    if (n2c->xy1.y == n1c->xy0.y) {
        // [U]P
        //  n2c
        // -----
        //  n1c
        u = true;
    }
    if (n1c->xy1.x == n2c->xy0.x) {
        // [R]IGHT
        //      |
        //  n1c | n2c
        //      |
        r = true;
    }
    if (n2c->xy1.x == n1c->xy0.x) {
        // [L]EFT
        //      |
        //  n2c | n1c
        //      |
        l = true;
    }
    if (d && !u && r && !l) {
        return RR_DOWN_RIGHT;
    } else if (!d && u && r && !l) {
        return RR_UP_RIGHT;
    } else if (!d && u && !r && l) {
        return RR_UP_LEFT;
    } else if (d && !u && !r && l) {
        return RR_DOWN_LEFT;
    } else if (d && !u && !r && !l) {
        return RR_DOWN;
    } else if (!d && u && !r && !l) {
        return RR_UP;
    } else if (!d && !u && r && !l) {
        return RR_RIGHT;
    } else if (!d && !u && !r && l) {
        return RR_LEFT;
    } else {
        LOGE("Unknown rect relation...");
        return RR_UNKNOWN;
    }
}


int ClampIntInclusiveExclusive(int v, int beg, int end) {
    if (beg >= end) {
        abort();
    }
    if (v < beg) {
        return beg;
    } else if (v >= end) {
        return end - 1;
    } else {
        return v;
    }
}

void AddNeighborByRectRelation(xy32xy32* n1c,
                               xy32xy32* n2c,
                               const ASNeighborList& neighbors,
                               xy32ibb* n,
                               pixel_waypoint_search* pws,
                               const size_t& next_i,
                               XYIBB_ENTER_EXIT next_ee,
                               bool add_nearest_only,
                               AStarNodeNeighborsCallback cb) {
    // insert pixel points "inside n2c" where points contact with one of n1c sides.
    LOGIx("AddNeighborByRectRelation Begin (%||,%||)[%||] ee=%|| -> next_i=%||, next_ee=%||", n->p.x, n->p.y, n->i, n->ee, next_i, next_ee);
    switch (rect_neighbor_relation(n1c, n2c)) {
    case RR_DOWN_RIGHT:
        AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy0.x, n2c->xy0.y, next_i, next_ee, XNC_COVER_ALL, cb);
        break;
    case RR_UP_RIGHT:
        AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy0.x, n2c->xy1.y - 1, next_i, next_ee, XNC_COVER_ALL, cb);
        break;
    case RR_UP_LEFT:
        AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy1.x - 1, n2c->xy1.y - 1, next_i, next_ee, XNC_COVER_ALL, cb);
        break;
    case RR_DOWN_LEFT:
        AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy1.x - 1, n2c->xy0.y, next_i, next_ee, XNC_COVER_ALL, cb);
        break;
    case RR_DOWN:
    {
        const int xbeg = std::max(n1c->xy0.x, n2c->xy0.x);
        const int xend = std::min(n1c->xy1.x, n2c->xy1.x);
        if (add_nearest_only) {
            int x_nearest = ClampIntInclusiveExclusive(n->p.x, xbeg, xend);
            for (int x = xbeg; x < xend; x++) {
                const int next_nc = XNC_COVER_NEAREST | ((x == xbeg) ? XNC_COVER_TO_FIRST : 0) | ((x == xend - 1) ? XNC_COVER_TO_LAST : 0);
                if (n->nc & XNC_COVER_TO_FIRST && x < x_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy0.y, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_NEAREST && x == x_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy0.y, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_TO_LAST && x > x_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy0.y, next_i, next_ee, next_nc, cb);
                }
            }
        } else {
            for (int x = xbeg; x < xend; x++) {
                AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy0.y, next_i, next_ee, XNC_COVER_ALL, cb);
            }
        }
        break;
    }
    case RR_UP:
    {
        const int xbeg = std::max(n1c->xy0.x, n2c->xy0.x);
        const int xend = std::min(n1c->xy1.x, n2c->xy1.x);
        if (add_nearest_only) {
            int x_nearest = ClampIntInclusiveExclusive(n->p.x, xbeg, xend);
            for (int x = xbeg; x < xend; x++) {
                const int next_nc = XNC_COVER_NEAREST | ((x == xbeg) ? XNC_COVER_TO_FIRST : 0) | ((x == xend - 1) ? XNC_COVER_TO_LAST : 0);
                if (n->nc & XNC_COVER_TO_FIRST && x < x_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy1.y - 1, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_NEAREST && x == x_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy1.y - 1, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_TO_LAST && x > x_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy1.y - 1, next_i, next_ee, next_nc, cb);
                }
            }
        } else {
            for (int x = xbeg; x < xend; x++) {
                AddPixelNeighborWithLog(neighbors, n, pws, x, n2c->xy1.y - 1, next_i, next_ee, XNC_COVER_ALL, cb);
            }
        }
        break;
    }
    case RR_RIGHT:
    {
        const int ybeg = std::max(n1c->xy0.y, n2c->xy0.y);
        const int yend = std::min(n1c->xy1.y, n2c->xy1.y);
        if (add_nearest_only) {
            int y_nearest = ClampIntInclusiveExclusive(n->p.y, ybeg, yend);
            for (int y = ybeg; y < yend; y++) {
                const int next_nc = XNC_COVER_NEAREST | ((y == ybeg) ? XNC_COVER_TO_FIRST : 0) | ((y == yend - 1) ? XNC_COVER_TO_LAST : 0);
                if (n->nc & XNC_COVER_TO_FIRST && y < y_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy0.x, y, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_NEAREST && y == y_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy0.x, y, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_TO_LAST && y > y_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy0.x, y, next_i, next_ee, next_nc, cb);
                }
            }
        } else {
            for (int y = ybeg; y < yend; y++) {
                AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy0.x, y, next_i, next_ee, XNC_COVER_ALL, cb);
            }
        }
        break;
    }

    case RR_LEFT:
    {
        const int ybeg = std::max(n1c->xy0.y, n2c->xy0.y);
        const int yend = std::min(n1c->xy1.y, n2c->xy1.y);
        if (add_nearest_only) {
            int y_nearest = ClampIntInclusiveExclusive(n->p.y, ybeg, yend);
            for (int y = ybeg; y < yend; y++) {
                const int next_nc = XNC_COVER_NEAREST | ((y == ybeg) ? XNC_COVER_TO_FIRST : 0) | ((y == yend - 1) ? XNC_COVER_TO_LAST : 0);
                if (n->nc & XNC_COVER_TO_FIRST && y < y_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy1.x - 1, y, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_NEAREST && y == y_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy1.x - 1, y, next_i, next_ee, next_nc, cb);
                } else if (n->nc & XNC_COVER_TO_LAST && y > y_nearest) {
                    AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy1.x - 1, y, next_i, next_ee, next_nc, cb);
                }
            }
        } else {
            for (int y = ybeg; y < yend; y++) {
                AddPixelNeighborWithLog(neighbors, n, pws, n2c->xy1.x - 1, y, next_i, next_ee, XNC_COVER_ALL, cb);
            }
        }
        break;
    }
    case RR_UNKNOWN:
    default:
        LOGE("Logic error for finding neighboring pixels...");
        abort();
        break;
    }
    LOGIx("AddNeighborByRectRelation End (%||,%||)[%||] ee=%|| -> next_i=%||, next_ee=%||", n->p.x, n->p.y, n->i, n->ee, next_i, next_ee);
}

void RTreePixelPathNodeNeighbors(ASNeighborList neighbors, void *node, void *context) {
    xy32ibb* n = reinterpret_cast<xy32ibb*>(node);
    pixel_waypoint_search* pws = reinterpret_cast<pixel_waypoint_search*>(context);
    ASPath cell_path = pws->cell_path;
    size_t cell_path_count = ASPathGetCount(cell_path);
    if (n->p.x == pws->to.x && n->p.y == pws->to.y) {
        // 'n' equals to 'to': reached endpoint (to-pixel)
        // no neighbors on endpoint
        return;
    } else if (n->i == cell_path_count - 1 && n->ee == XEE_ENTER) {
        // all last cells' pixels (except endpoint pixel) always have the only neighbor: endpoint pixel
        AddPixelNeighborWithLog(neighbors, n, pws, pws->to.x, pws->to.y, cell_path_count - 1, XEE_EXIT, XNC_NO_COVERAGE, RTreePixelPathNodeNeighbors);
        return;
    }
    xy32xy32* n1c;
    xy32xy32* n2c;
    XYIBB_ENTER_EXIT next_ee = XEE_ENTER;
    size_t next_i = 0;
    if (n->ee == XEE_ENTER) {
        // Get exit nodes at the same cell node
        n1c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 1)); // Next Cell node
        n2c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 0)); // Cell node containing 'n'
        next_ee = XEE_EXIT;
        next_i = n->i; // the same cell node
    } else if (n->ee == XEE_EXIT) {
        // Get enter nodes at the next cell node
        n1c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 0)); // Cell node containing 'n'
        n2c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 1)); // Next Cell node
        next_ee = XEE_ENTER;
        next_i = n->i + 1; // the next cell node
    } else {
        LOGE("Corrupted ee");
        abort();
    }
    AddNeighborByRectRelation(n1c,
                              n2c,
                              neighbors,
                              n,
                              pws,
                              next_i,
                              next_ee,
                              false,
                              RTreePixelPathNodeNeighbors);
}

void RTreePixelPathNodeNeighborsSuboptimal(ASNeighborList neighbors, void *node, void *context) {
    xy32ibb* n = reinterpret_cast<xy32ibb*>(node);
    pixel_waypoint_search* pws = reinterpret_cast<pixel_waypoint_search*>(context);
    ASPath cell_path = pws->cell_path;
    size_t cell_path_count = ASPathGetCount(cell_path);
    printf("Finding pixel neighbors on cell index %lld...\n", n->i);
    if (n->p.x == pws->to.x && n->p.y == pws->to.y) {
        // 'n' equals to 'to': reached endpoint (to-pixel)
        // no neighbors on endpoint
        return;
    } else if (n->i == cell_path_count - 1 && n->ee == XEE_ENTER) {
        // all last cells' pixels (except endpoint pixel) always have the only neighbor: endpoint pixel
        AddPixelNeighborWithLog(neighbors, n, pws, pws->to.x, pws->to.y, cell_path_count - 1, XEE_EXIT, XNC_NO_COVERAGE, RTreePixelPathNodeNeighborsSuboptimal);
        return;
    }
    xy32xy32* n1c;
    xy32xy32* n2c;
    XYIBB_ENTER_EXIT next_ee = XEE_ENTER;
    size_t next_i = 0;
    if (n->ee == XEE_ENTER) {
        // Get exit nodes at the same cell node (cell n->i)
        n1c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 1)); // Next Cell node
        n2c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 0)); // Cell node containing 'n' (neighbors are inside this cell)
        next_ee = XEE_EXIT;
        next_i = n->i; // the same cell node
    } else if (n->ee == XEE_EXIT) {
        // Get enter nodes at the next cell node (cell n->i + 1)
        n1c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 0)); // Cell node containing 'n'
        n2c = reinterpret_cast<xy32xy32*>(ASPathGetNode(cell_path, n->i + 1)); // Next Cell node (neighbors are inside this cell)
        next_ee = XEE_ENTER;
        next_i = n->i + 1; // the next cell node
    } else {
        LOGE("Corrupted ee");
        abort();
    }
    const bool add_nearest_only = !(n->i == 0 && n->ee == XEE_ENTER);
    AddNeighborByRectRelation(n1c,
                              n2c,
                              neighbors,
                              n,
                              pws,
                              next_i,
                              next_ee,
                              true,
                              RTreePixelPathNodeNeighborsSuboptimal);
}

float RTreePixelPathNodeHeuristic(void *fromNode, void *toNode, void *context) {
    xy32ibb* from = reinterpret_cast<xy32ibb*>(fromNode);
    xy32ibb* to = reinterpret_cast<xy32ibb*>(toNode);
    return xyib_distance(*from, *to);
}

int RTreePixelPathNodeComparator(void *node1, void *node2, void *context) {
    pixel_waypoint_search* pws = reinterpret_cast<pixel_waypoint_search*>(context);
    xy32ibb* n1 = reinterpret_cast<xy32ibb*>(node1);
    xy32ibb* n2 = reinterpret_cast<xy32ibb*>(node2);
    // no comparison on 'ee'-bit when it is goal node
    if (n1->p.x == n2->p.x && n2->p.x == pws->to.x && n1->p.y == n2->p.y && n2->p.y == pws->to.y) {
        return 0;
    }
    int64_t n1v = static_cast<int64_t>(n1->p.y) << (32 + 1) | n1->p.x << 1 | static_cast<int64_t>(n1->ee);
    int64_t n2v = static_cast<int64_t>(n2->p.y) << (32 + 1) | n2->p.x << 1 | static_cast<int64_t>(n2->ee);
    int64_t d = n1v - n2v;
    if (d == 0) {
        return 0;
    } else if (d > 0) {
        return 1;
    } else {
        return -1;
    }
}

box astarrtree::box_t_from_xy(xy32 v) {
    return box(point(v.x, v.y), point(v.x + 1, v.y + 1));
}

xy32xy32 xyxy_from_xy(xy32 v) {
    return xy32xy32{ { v.x, v.y },{ v.x + 1, v.y + 1 } };
}

static int RTreePathNodeEarlyExit(size_t visitedCount, void *visitingNode, void *goalNode, void *context) {
    auto pathfind_context = reinterpret_cast<PathfindContext*>(context);
    pathfind_context->early_exit_call_count++;
    if (visitedCount >= 200000) {
        LOGE("Too many visits! (%1% visits) Pathfinding aborted.", visitedCount);
        return -1;
    } else {
        if (pathfind_context->coro && pathfind_context->early_exit_call_count % 1000 == 0) {
            /*boost::asio::deadline_timer timer(pathfind_context->coro->io_service);
            timer.expires_from_now(boost::posix_time::milliseconds(10));
            timer.async_wait(pathfind_context->coro->yield);*/
        }
        const auto visiting = reinterpret_cast<const xy32xy32xy32*>(visitingNode);
        const auto goal = reinterpret_cast<const xy32xy32xy32*>(goalNode);
        //printf("%d\n", (int)visitedCount);
        /*LOGI("%|| visits so far... visiting:%||,%||[%||,%||-%||,%||] / goal:%||,%||[%||,%||-%||,%||]",
        visitedCount,
        visiting->point.x,
        visiting->point.y,
        visiting->box.xy0.x,
        visiting->box.xy0.y,
        visiting->box.xy1.x,
        visiting->box.xy1.y,
        goal->point.x,
        goal->point.y,
        goal->box.xy0.x,
        goal->box.xy0.y,
        goal->box.xy1.x,
        goal->box.xy1.y);*/
    }
    return 0;
}

static int RTreePixelPathNodeEarlyExit(size_t visitedCount, void *visitingNode, void *goalNode, void *context) {
    pixel_waypoint_search* pws = reinterpret_cast<pixel_waypoint_search*>(context);

    if (visitedCount >= 100000) {
        LOGE("Too many pixel visits! (%1% visits) Pixel-level pathfinding aborted.", visitedCount);
        return -1;
    } else {
        if (pws->coro && pws->early_exit_call_count % 1000 == 0) {
            /*boost::asio::deadline_timer timer(pws->coro->io_service);
            timer.expires_from_now(boost::posix_time::milliseconds(10));
            timer.async_wait(pws->coro->yield);*/
        }
        const auto visiting = reinterpret_cast<const xy32xy32xy32*>(visitingNode);
        const auto goal = reinterpret_cast<const xy32xy32xy32*>(goalNode);
        //printf("pixel-level visit count = %lld\n", visitedCount);
        //printf("%d\n", (int)visitedCount);
        /*LOGI("%|| visits so far... visiting:%||,%||[%||,%||-%||,%||] / goal:%||,%||[%||,%||-%||,%||]",
        visitedCount,
        visiting->point.x,
        visiting->point.y,
        visiting->box.xy0.x,
        visiting->box.xy0.y,
        visiting->box.xy1.x,
        visiting->box.xy1.y,
        goal->point.x,
        goal->point.y,
        goal->box.xy0.x,
        goal->box.xy0.y,
        goal->box.xy1.x,
        goal->box.xy1.y);*/
    }
    return 0;
}

std::vector<xy32> calculate_pixel_waypoints(const xy32 from,
                                            const xy32 to,
                                            const ASPath cell_path,
                                            std::shared_ptr<coro_context> coro,
                                            const double distance) {
    std::vector<xy32> waypoints;
    ASPathNodeSource PathNodeSource =
    {
        sizeof(xy32ibb),
        RTreePixelPathNodeNeighbors,
        RTreePixelPathNodeHeuristic,
        RTreePixelPathNodeEarlyExit,
        RTreePixelPathNodeComparator
    };
    size_t cell_path_count = ASPathGetCount(cell_path);
    if (cell_path_count == 0) {
        LOGE("calculate_pixel_waypoints: cell_path_count is 0.");
        abort();
    }
    xy32ibb from_rect = { from, 0, XEE_ENTER, XNC_COVER_ALL };
    xy32ibb to_rect = { to, cell_path_count - 1, XEE_EXIT, XNC_NO_COVERAGE };
    pixel_waypoint_search pws = {
        from,
        to,
        cell_path,
        coro,
        0,
    };
    ASPath pixel_path = ASPathCreate(&PathNodeSource, &pws, &from_rect, &to_rect);
    size_t pixel_path_count = ASPathGetCount(pixel_path);
    if (pixel_path_count > 0) {
        LOGI("Pixel Path Count: %1%", pixel_path_count);
        float pixel_path_cost = ASPathGetCost(pixel_path);
        LOGI("Pixel Path Cost: %f", pixel_path_cost);
        if (distance > pixel_path_cost + 1e-4) {
            LOGEP("Pixel path cost %f is less than distance %f!!! LOGIC ERROR", pixel_path_cost, distance);
        }
        for (size_t i = 0; i < pixel_path_count; i++) {
            xy32ibb* pixel_node = reinterpret_cast<xy32ibb*>(ASPathGetNode(pixel_path, i));
            LOGIx("Pixel Path %1%: (%2%, %3%) [Cell index=%4%]",
                    i,
                    pixel_node->p.x,
                    pixel_node->p.y,
                    pixel_node->i);
            waypoints.push_back(pixel_node->p);
        }
    } else {
        LOGE("No pixel waypoints found.");
    }
    ASPathDestroy(pixel_path);
    return waypoints;
}

bool astarrtree::find_nearest_point_if_empty(const rtree* rtree_ptr, xy32& from, box& from_box, std::vector<value>& from_result_s) {
    if (from_result_s.size() == 0) {
        auto nearest_it = rtree_ptr->qbegin(bgi::nearest(from_box, 1));
        if (nearest_it == rtree_ptr->qend()) {
            LOGE("Empty result from nearest query...");
            abort();
        }
        auto nearest_xyxy = xyxy_from_box_t(nearest_it->first);
        auto from_xyxy = xyxy_from_xy(from);
        switch (rect_relation(&nearest_xyxy, &from_xyxy)) {
        case RR_DOWN_RIGHT:
            from = { nearest_xyxy.xy1.x - 1, nearest_xyxy.xy1.y - 1 };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_UP_RIGHT:
            from = { nearest_xyxy.xy1.x - 1, nearest_xyxy.xy0.y };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_UP_LEFT:
            from = { nearest_xyxy.xy0.x, nearest_xyxy.xy0.y };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_DOWN_LEFT:
            from = { nearest_xyxy.xy0.x, nearest_xyxy.xy1.y - 1 };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_DOWN:
            from = { from.x, nearest_xyxy.xy1.y - 1 };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_UP:
            from = { from.x, nearest_xyxy.xy0.y };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_RIGHT:
            from = { nearest_xyxy.xy1.x - 1, from.y };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_LEFT:
            from = { nearest_xyxy.xy0.x, from.y };
            from_box = box_t_from_xy(from);
            from_result_s.push_back(*nearest_it);
            break;
        case RR_UNKNOWN:
        default:
            LOGE("rect relation unknown...");
            abort();
            break;
        }
        return true;
    } else {
        return false;
    }
}

std::vector<xy32> astarrtree::astar_rtree_memory(const rtree* rtree_ptr, const xy32& from, const xy32& to, std::shared_ptr<coro_context> coro) {
    const double distance = xy32_distance(from, to);
    LOGI("Pathfinding from (%1%,%2%) -> (%3%,%4%) [distance = %5%]",
         from.x,
         from.y,
         to.x,
         to.y,
         distance);
    std::vector<xy32> waypoints;
    LOGI("R Tree size: %1%", rtree_ptr->size());
    if (rtree_ptr->size() == 0) {
        return waypoints;
    }

    auto from_box = box_t_from_xy(from);
    std::vector<value> from_result_s;
    rtree_ptr->query(bgi::contains(from_box), std::back_inserter(from_result_s));
    //if (astarrtree::find_nearest_point_if_empty(rtree_ptr, from, from_box, from_result_s)) {
    //    LOGI("  'From' point changed to (%1%,%2%)",
    //         from.x,
    //         from.y);
    //}

    auto to_box = box_t_from_xy(to);
    std::vector<value> to_result_s;
    rtree_ptr->query(bgi::contains(to_box), std::back_inserter(to_result_s));
    //if (astarrtree::find_nearest_point_if_empty(rtree_ptr, to, to_box, to_result_s)) {
    //    LOGI("  'To' point changed to (%1%,%2%)",
    //         to.x,
    //         to.y);
    //}

    if (from_result_s.size() == 1 && to_result_s.size() == 1) {
        // Phase 1 - R Tree rectangular node searching
        ASPathNodeSource PathNodeSource =
        {
            sizeof(xy32xy32xy32),
            RTreePathNodeNeighbors,
            RTreePathNodeHeuristic,
            RTreePathNodeEarlyExit,
            RTreePathNodeComparator
        };
        const auto from_box = xyxy_from_box_t(from_result_s[0].first);
        const auto to_box = xyxy_from_box_t(to_result_s[0].first);
        xy32xy32xy32 from_rect = { from_box, from };
        xy32xy32xy32 to_rect = { to_box, to };
        PathfindContext context{
            { { from.x,from.y },{ from.x + 1, from.y + 1 } },
            { { to.x,to.y },{ to.x + 1, to.y + 1 } },
            from_box,
            to_box,
            rtree_ptr,
            coro,
            0,
        };
        ASPath path = ASPathCreate(&PathNodeSource, &context, &from_rect, &to_rect);
        size_t cell_path_count = ASPathGetCount(path);
        if (cell_path_count > 0) {
            LOGI("Cell Path Count: %1%", cell_path_count);
            float cell_path_cost = ASPathGetCost(path);
            LOGI("Cell Path Cost: %f", cell_path_cost);
            // need some margin for almost equal values
            if (cell_path_count > 1) {
                if (distance > cell_path_cost + 1e-4) {
                    LOGEP("Cell path cost %f is less than distance %f!!! LOGIC ERROR", cell_path_cost, distance);
                }
            } else {
                LOGI("Cell path cost is '0' since cell path count is 1 (pathfinding done within single cell)");
            }
            for (size_t i = 0; i < cell_path_count; i++) {
                xy32xy32* node = reinterpret_cast<xy32xy32*>(ASPathGetNode(path, i));
                LOGI("Cell Path %1%: (%2%, %3%)-(%4%, %5%) [%6% x %7% = %8%]",
                        i,
                        node->xy0.x,
                        node->xy0.y,
                        node->xy1.x,
                        node->xy1.y,
                        node->xy1.x - node->xy0.x,
                        node->xy1.y - node->xy0.y,
                        (node->xy1.x - node->xy0.x) * (node->xy1.y - node->xy0.y));
            }
            // Phase 2 - per-pixel node searching
            waypoints = calculate_pixel_waypoints(from, to, path, coro, distance);
        } else {
            LOGE("No path found.");
        }
        ASPathDestroy(path);
    } else {
        LOGE("From-node and/or to-node error.");
    }
    return waypoints;
}
