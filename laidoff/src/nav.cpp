#include <stdlib.h>
#include <string.h>
#include <functional> // std::bind
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "nav.h"
#include "file.h"
#include "lwlog.h"
#include "lwmacro.h"
#include "field.h"
#include "pcg_basic.h"

#define MAX_PATHQUERY_POLYS (256)
#define MAX_PATH_QUERY (100)

typedef struct _LWPATHQUERY {
    // Valid flag
    int valid;
    // Update positon flag
    int update_output;
    // Start position
    float spos[3];
    // End position
    float epos[3];
    // Start position ref
    dtPolyRef start_ref;
    // End position ref
    dtPolyRef end_ref;
    // Smooth path points
    float smooth_path[MAX_PATHQUERY_SMOOTH * 3];
    // Total smooth path point count
    int n_smooth_path;
    // Abstract time for test player movement
    float path_t;
} LWPATHQUERY;

typedef struct _LWNAV {
    dtNavMesh* nav_mesh;
    dtNavMeshQuery* nav_query;
    float poly_pick_ext[3];
    dtQueryFilter filter;
    dtPolyRef polys[MAX_PATHQUERY_POLYS];
    int n_polys;
    // Path query result for testing
    LWPATHQUERY path_query_test;
    // Path query result array
    LWPATHQUERY path_query[MAX_PATH_QUERY];
    // Output location (pointer to vec3)
    float* path_query_output_location[MAX_PATH_QUERY];
    // Output orientation (pointer to float)
    float* path_query_output_orientation[MAX_PATH_QUERY];
    // Random number generator instance
    pcg32_random_t rng;
} LWNAV;

enum SamplePolyAreas {
    SAMPLE_POLYAREA_GROUND,
    SAMPLE_POLYAREA_WATER,
    SAMPLE_POLYAREA_ROAD,
    SAMPLE_POLYAREA_DOOR,
    SAMPLE_POLYAREA_GRASS,
    SAMPLE_POLYAREA_JUMP,
};
enum SamplePolyFlags {
    SAMPLE_POLYFLAGS_WALK = 0x01,       // Ability to walk (ground, grass, road)
    SAMPLE_POLYFLAGS_SWIM = 0x02,       // Ability to swim (water).
    SAMPLE_POLYFLAGS_DOOR = 0x04,       // Ability to move through doors.
    SAMPLE_POLYFLAGS_JUMP = 0x08,       // Ability to jump.
    SAMPLE_POLYFLAGS_DISABLED = 0x10,       // Disabled polygon
    SAMPLE_POLYFLAGS_ALL = 0xffff   // All abilities.
};

// return [0, 1) float
float s_random_float(pcg32_random_t* rng) {
    uint32_t v = pcg32_random_r(rng);
    //uint32_t v = pcg32_random();
    //LOGI("v = 0x%08x", v);
    return static_cast<float>(ldexp(v, -32));
}

void* load_nav(const char* filename) {
    auto nav = new LWNAV();
    nav->nav_mesh = dtAllocNavMesh();
    
    size_t size = 0;
    char* d = create_binary_from_file(filename, &size);

    dtStatus status;
    status = nav->nav_mesh->init(reinterpret_cast<unsigned char*>(d), static_cast<int>(size), DT_TILE_FREE_DATA);
    if (dtStatusFailed(status)) {
        LOGE("nav->nav_mesh->init() error");
    }

    nav->nav_query = dtAllocNavMeshQuery();

    status = nav->nav_query->init(nav->nav_mesh, 2048);
    if (dtStatusFailed(status)) {
        LOGE("nav->nav_query->init() error");
    }

    nav->poly_pick_ext[0] = 2;
    nav->poly_pick_ext[1] = 4;
    nav->poly_pick_ext[2] = 2;

    nav->filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
    nav->filter.setExcludeFlags(0);
    nav->filter.setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);
    nav->filter.setAreaCost(SAMPLE_POLYAREA_WATER, 10.0f);
    nav->filter.setAreaCost(SAMPLE_POLYAREA_ROAD, 1.0f);
    nav->filter.setAreaCost(SAMPLE_POLYAREA_DOOR, 1.0f);
    nav->filter.setAreaCost(SAMPLE_POLYAREA_GRASS, 2.0f);
    nav->filter.setAreaCost(SAMPLE_POLYAREA_JUMP, 1.5f);

    
    return nav;
}

void unload_nav(LWNAV* nav) {
    dtFreeNavMeshQuery(nav->nav_query);
    dtFreeNavMesh(nav->nav_mesh);
    delete nav;
}

inline bool inRange(const float* v1, const float* v2, const float r, const float h) {
    const float dx = v2[0] - v1[0];
    const float dy = v2[1] - v1[1];
    const float dz = v2[2] - v1[2];
    return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
}

static bool getSteerTarget(dtNavMeshQuery* navQuery, const float* startPos, const float* endPos,
    const float minTargetDist,
    const dtPolyRef* path, const int pathSize,
    float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
    float* outPoints = 0, int* outPointCount = 0) {
    // Find steer target.
    static const int MAX_STEER_POINTS = 3;
    float steerPath[MAX_STEER_POINTS * 3];
    unsigned char steerPathFlags[MAX_STEER_POINTS];
    dtPolyRef steerPathPolys[MAX_STEER_POINTS];
    int nsteerPath = 0;
    navQuery->findStraightPath(startPos, endPos, path, pathSize,
        steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);
    if (!nsteerPath)
        return false;

    if (outPoints && outPointCount) {
        *outPointCount = nsteerPath;
        for (int i = 0; i < nsteerPath; ++i)
            dtVcopy(&outPoints[i * 3], &steerPath[i * 3]);
    }


    // Find vertex far enough to steer to.
    int ns = 0;
    while (ns < nsteerPath) {
        // Stop at Off-Mesh link or when point is further than slop away.
        if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
            !inRange(&steerPath[ns * 3], startPos, minTargetDist, 1000.0f))
            break;
        ns++;
    }
    // Failed to find good point to steer to.
    if (ns >= nsteerPath)
        return false;

    dtVcopy(steerPos, &steerPath[ns * 3]);
    steerPos[1] = startPos[1];
    steerPosFlag = steerPathFlags[ns];
    steerPosRef = steerPathPolys[ns];

    return true;
}

static int fixupCorridor(dtPolyRef* path, const int npath, const int maxPath,
    const dtPolyRef* visited, const int nvisited) {
    int furthestPath = -1;
    int furthestVisited = -1;

    // Find furthest common polygon.
    for (int i = npath - 1; i >= 0; --i) {
        bool found = false;
        for (int j = nvisited - 1; j >= 0; --j) {
            if (path[i] == visited[j]) {
                furthestPath = i;
                furthestVisited = j;
                found = true;
            }
        }
        if (found)
            break;
    }

    // If no intersection found just return current path. 
    if (furthestPath == -1 || furthestVisited == -1)
        return npath;

    // Concatenate paths.   

    // Adjust beginning of the buffer to include the visited.
    const int req = nvisited - furthestVisited;
    const int orig = LWMIN(furthestPath + 1, npath);
    int size = LWMAX(0, npath - orig);
    if (req + size > maxPath)
        size = maxPath - req;
    if (size)
        memmove(path + req, path + orig, size * sizeof(dtPolyRef));

    // Store visited
    for (int i = 0; i < req; ++i)
        path[i] = visited[(nvisited - 1) - i];

    return req + size;
}


// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary, 
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
static int fixupShortcuts(dtPolyRef* path, int npath, dtNavMeshQuery* navQuery) {
    if (npath < 3)
        return npath;

    // Get connected polygons
    static const int maxNeis = 16;
    dtPolyRef neis[maxNeis];
    int nneis = 0;

    const dtMeshTile* tile = 0;
    const dtPoly* poly = 0;
    if (dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
        return npath;

    for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next) {
        const dtLink* link = &tile->links[k];
        if (link->ref != 0) {
            if (nneis < maxNeis)
                neis[nneis++] = link->ref;
        }
    }

    // If any of the neighbour polygons is within the next few polygons
    // in the path, short cut to that polygon directly.
    static const int maxLookAhead = 6;
    int cut = 0;
    for (int i = dtMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--) {
        for (int j = 0; j < nneis; j++) {
            if (path[i] == neis[j]) {
                cut = i;
                break;
            }
        }
    }
    if (cut > 1) {
        int offset = cut - 1;
        npath -= offset;
        for (int i = 1; i < npath; i++)
            path[i] = path[i + offset];
    }

    return npath;
}

void nav_query(LWNAV* nav, LWPATHQUERY* pq) {
    nav->nav_query->findNearestPoly(pq->spos, nav->poly_pick_ext, &nav->filter, &pq->start_ref, 0);
    nav->nav_query->findNearestPoly(pq->epos, nav->poly_pick_ext, &nav->filter, &pq->end_ref, 0);
    nav->nav_query->findPath(pq->start_ref, pq->end_ref, pq->spos, pq->epos, &nav->filter, nav->polys, &nav->n_polys, MAX_PATHQUERY_POLYS);

    pq->n_smooth_path = 0;

    if (nav->n_polys) {
        // Iterate over the path to find smooth path on the detail mesh surface.
        dtPolyRef polys[MAX_PATHQUERY_POLYS];
        memcpy(polys, nav->polys, sizeof(dtPolyRef)*nav->n_polys);
        int npolys = nav->n_polys;

        float iterPos[3], targetPos[3];
        nav->nav_query->closestPointOnPoly(pq->start_ref, pq->spos, iterPos, 0);
        nav->nav_query->closestPointOnPoly(polys[npolys - 1], pq->epos, targetPos, 0);

        static const float STEP_SIZE = 0.5f;
        static const float SLOP = 0.01f;

        pq->n_smooth_path = 0;

        dtVcopy(&pq->smooth_path[pq->n_smooth_path * 3], iterPos);
        pq->n_smooth_path++;

        // Move towards target a small advancement at a time until target reached or
        // when ran out of memory to store the path.
        while (npolys && pq->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
            // Find location to steer towards.
            float steerPos[3];
            unsigned char steerPosFlag;
            dtPolyRef steerPosRef;

            if (!getSteerTarget(nav->nav_query, iterPos, targetPos, SLOP,
                polys, npolys, steerPos, steerPosFlag, steerPosRef))
                break;

            bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
            bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

            // Find movement delta.
            float delta[3], len;
            dtVsub(delta, steerPos, iterPos);
            len = dtMathSqrtf(dtVdot(delta, delta));
            // If the steer target is end of path or off-mesh link, do not move past the location.
            if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
                len = 1;
            else
                len = STEP_SIZE / len;
            float moveTgt[3];
            dtVmad(moveTgt, iterPos, delta, len);

            // Move
            float result[3];
            dtPolyRef visited[16];
            int nvisited = 0;
            nav->nav_query->moveAlongSurface(polys[0], iterPos, moveTgt, &nav->filter,
                result, visited, &nvisited, 16);

            npolys = fixupCorridor(polys, npolys, MAX_PATHQUERY_POLYS, visited, nvisited);
            npolys = fixupShortcuts(polys, npolys, nav->nav_query);

            float h = 0;
            nav->nav_query->getPolyHeight(polys[0], result, &h);
            result[1] = h;
            dtVcopy(iterPos, result);

            // Handle end of path and off-mesh links when close enough.
            if (endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f)) {
                // Reached end of path.
                dtVcopy(iterPos, targetPos);
                if (pq->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
                    dtVcopy(&pq->smooth_path[pq->n_smooth_path * 3], iterPos);
                    pq->n_smooth_path++;
                }
                break;
            } else if (offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f)) {
                // Reached off-mesh connection.
                float startPos[3], endPos[3];

                // Advance the path up to and over the off-mesh connection.
                dtPolyRef prevRef = 0, polyRef = polys[0];
                int npos = 0;
                while (npos < npolys && polyRef != steerPosRef) {
                    prevRef = polyRef;
                    polyRef = polys[npos];
                    npos++;
                }
                for (int i = npos; i < npolys; ++i)
                    polys[i - npos] = polys[i];
                npolys -= npos;

                // Handle the connection.
                dtStatus status = nav->nav_mesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
                if (dtStatusSucceed(status)) {
                    if (pq->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
                        dtVcopy(&pq->smooth_path[pq->n_smooth_path * 3], startPos);
                        pq->n_smooth_path++;
                        // Hack to make the dotted path not visible during off-mesh connection.
                        if (pq->n_smooth_path & 1) {
                            dtVcopy(&pq->smooth_path[pq->n_smooth_path * 3], startPos);
                            pq->n_smooth_path++;
                        }
                    }
                    // Move position at the other side of the off-mesh link.
                    dtVcopy(iterPos, endPos);
                    float eh = 0.0f;
                    nav->nav_query->getPolyHeight(polys[0], iterPos, &eh);
                    iterPos[1] = eh;
                }
            }

            // Store results.
            if (pq->n_smooth_path < MAX_PATHQUERY_SMOOTH) {
                dtVcopy(&pq->smooth_path[pq->n_smooth_path * 3], iterPos);
                pq->n_smooth_path++;
            }
        }
    }
}

int nav_update_path_query(LWNAV* nav, LWPATHQUERY* pq, float move_speed, float delta_time, float* out_pos, float* out_rot) {
    if (pq->n_smooth_path) {
        pq->path_t += delta_time;
        const float idx_f = fmodf((float)(pq->path_t * move_speed), (float)pq->n_smooth_path);
        const int idx = (int)idx_f;
        const float* p1 = &pq->smooth_path[3 * idx];
        // path query result's coordinates is different from world coordinates.
        const vec3 p1vec = { p1[0], -p1[2], p1[1] };
        if (idx < pq->n_smooth_path - 1) {
            const float* p2 = &pq->smooth_path[3 * (idx + 1)];
            // path query result's coordinates is different from world coordinates.
            const vec3 p2vec = { p2[0], -p2[2], p2[1] };
            // Calculate a midpoint between p1 and p2 to get interpolated position according to idx_f value
            const float p2coeff = idx_f - idx;
            const float p1coeff = 1.0f - p2coeff;
            const vec3 p1p2midvec = {
                p1vec[0] * p1coeff + p2vec[0] * p2coeff,
                p1vec[1] * p1coeff + p2vec[1] * p2coeff,
                p1vec[2] * p1coeff + p2vec[2] * p2coeff,
            };
            // Update result position
            if (out_pos) {
                memcpy(out_pos, p1p2midvec, sizeof(vec3));
            }
            // Update result orientation
            if (out_rot) {
                *out_rot = atan2f(p2vec[1] - p1vec[1], p2vec[0] - p1vec[0]);
            }
        } else {
            if (out_pos) {
                memcpy(out_pos, p1vec, sizeof(vec3));
            }
        }
        // Start querying another random path after the current pathfinding is completed
        if (idx >= pq->n_smooth_path - 1) {
            start_new_path_query_continue_test(nav, pq);
        }
        return 0;
    }
    // Navigation data is not initialized
    return -1;
}

void pathquery_set_start(LWPATHQUERY* pq, dtPolyRef start_ref, const float* spos) {
    memcpy(pq->spos, spos, sizeof(float) * 3);
    pq->start_ref = start_ref;
}

void pathquery_set_end(LWPATHQUERY* pq, dtPolyRef end_ref, const float* epos) {
    memcpy(pq->epos, epos, sizeof(float) * 3);
    pq->end_ref = end_ref;
}

void pathquery_set_start_to_end(LWPATHQUERY* pq) {
    pathquery_set_start(pq, pq->end_ref, pq->epos);
}

void pathquery_start_end(LWPATHQUERY* pq, dtPolyRef* start_ref, float* spos, dtPolyRef* end_ref, float* epos) {
    *start_ref = pq->start_ref;
    memcpy(spos, pq->spos, sizeof(float) * 3);
    *end_ref = pq->end_ref;
    memcpy(epos, pq->epos, sizeof(float) * 3);
}

void* nav_path_query_test(LWNAV* nav) {
    return &nav->path_query_test;
}

void* nav_path_query(LWNAV* nav, int idx) {
    if (idx < 0 || idx >= MAX_PATH_QUERY) {
        LOGEP("index error");
        return 0;
    }
    return &nav->path_query[idx];
}

int nav_new_path_query(LWNAV* nav) {
    for (int i = 0; i < MAX_PATH_QUERY; i++) {
        if (nav->path_query[i].valid) {
            continue;
        }
        nav->path_query[i].valid = 1;
        start_new_path_query_test(nav, &nav->path_query[i]);
        return i;
    }
    LOGEP("preallocated pool exceeded error");
    return -1;
}

void nav_clear_all_path_queries(LWNAV* nav) {
    memset(&nav->path_query_test, 0, sizeof(nav->path_query_test));
    memset(nav->path_query, 0, sizeof(nav->path_query));
}

void nav_reset_deterministic_seed(LWNAV* nav) {
    // Seed a random number generator
    pcg32_srandom_r(&nav->rng, 0x0DEEC2CBADF00D77, 0x15881588CA11DAC1);
}

int nav_update_output_path_query(LWNAV* nav, int idx, int val) {
    if (idx < 0 || idx >= MAX_PATH_QUERY) {
        LOGEP("index error");
        return -1;
    }
    if (!nav->path_query[idx].valid) {
        LOGEP("invalid entry error");
        return -1;
    }
    nav->path_query[idx].update_output = val;
    return 0;
}

int nav_bind_path_query_output_location(LWNAV* nav, int idx, LWFIELD* field, int field_object_idx) {
    if (idx < 0 || idx >= MAX_PATH_QUERY) {
        LOGEP("path query index error");
        return -1;
    }
    if (!nav->path_query[idx].valid) {
        LOGEP("invalid entry error");
        return -1;
    }
    nav->path_query_output_location[idx] = field_field_object_location_rawptr(field, field_object_idx);
    nav->path_query_output_orientation[idx] = field_field_object_orientation_rawptr(field, field_object_idx);
    return 0;
}

void nav_update(LWNAV* nav, float move_speed, float delta_time) {
    for (int i = 0; i < MAX_PATH_QUERY; i++) {
        if (nav->path_query[i].valid && nav->path_query[i].update_output) {
            nav_update_path_query(nav, &nav->path_query[i], move_speed, delta_time,
                nav->path_query_output_location[i], nav->path_query_output_orientation[i]);
            // Jump jump!
            const float nav_jump_height = 0.7f;
            nav->path_query_output_location[i][2] += nav_jump_height * fabs(sinf(nav->path_query[i].path_t * 11));
        }
    }
}

void nav_path_query_spos(const LWNAV* nav, float* p) {
    p[0] = nav->path_query_test.spos[0];
    p[1] = -nav->path_query_test.spos[2];
    p[2] = nav->path_query_test.spos[1];
}

void nav_path_query_epos(const LWNAV* nav, float* p) {
    p[0] = nav->path_query_test.epos[0];
    p[1] = -nav->path_query_test.epos[2];
    p[2] = nav->path_query_test.epos[1];
}

void nav_set_path_query_spos(LWNAV* nav, float x, float y, float z) {
    nav->path_query_test.spos[0] = x;
    nav->path_query_test.spos[1] = z;
    nav->path_query_test.spos[2] = -y;
}

void nav_set_path_query_epos(LWNAV* nav, float x, float y, float z) {
    nav->path_query_test.epos[0] = x;
    nav->path_query_test.epos[1] = z;
    nav->path_query_test.epos[2] = -y;
}

int nav_path_query_n_smooth_path(const LWNAV* nav) {
    return nav->path_query_test.n_smooth_path;
}


void set_random_start_end_pos(LWNAV* nav, LWPATHQUERY* pq) {
    dtPolyRef start_ref, end_ref;
    float spos[3], epos[3];
    auto frand = std::bind(s_random_float, &nav->rng);
    
    dtStatus status = nav->nav_query->findRandomPoint(&nav->filter, frand, &start_ref, spos);
    if (dtStatusFailed(status)) {
        LOGE("nav->nav_query->findRandomPoint() error");
    }

    status = nav->nav_query->findRandomPoint(&nav->filter, frand, &end_ref, epos);
    if (dtStatusFailed(status)) {
        LOGE("nav->nav_query->findRandomPoint() error");
    }

    pathquery_set_start(pq, start_ref, spos);
    pathquery_set_end(pq, end_ref, epos);
}

void set_random_next_pos(LWNAV* nav, LWPATHQUERY* pq) {
    // Set current end position as start position
    pathquery_set_start_to_end(pq);

    dtPolyRef end_ref;
    float epos[3];
    auto frand = std::bind(s_random_float, &nav->rng);
    dtStatus status = nav->nav_query->findRandomPoint(&nav->filter, frand, &end_ref, epos);
    if (dtStatusFailed(status)) {
        LOGE("nav->nav_query->findRandomPoint() error");
    }

    pathquery_set_end(pq, end_ref, epos);
}

void start_new_path_query_test(LWNAV* nav, LWPATHQUERY* pq) {
    set_random_start_end_pos(nav, pq);
    nav_query(nav, pq);
    pq->path_t = 0;
}

void start_new_path_query_continue_test(LWNAV* nav, LWPATHQUERY* pq) {
    set_random_next_pos(nav, pq);
    nav_query(nav, pq);
    pq->path_t = 0;
}

void reset_nav_context(LWNAV* nav) {
    // Clear all path queries
    nav_clear_all_path_queries(nav);
    // Reset random generator seed
    nav_reset_deterministic_seed(nav);
    // Start test player nav
    start_new_path_query_test(nav, &nav->path_query_test);
}
