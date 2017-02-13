#ifndef QTPFS_DEFINES_HDR
#define QTPFS_DEFINES_HDR

// #define QTPFS_NO_LOADSCREEN
// #define QTPFS_IGNORE_DEAD_PATHS
#define QTPFS_LIMIT_TEAM_SEARCHES
#define QTPFS_SUPPORT_PARTIAL_SEARCHES
// #define QTPFS_TRACE_PATH_SEARCHES
#define QTPFS_SEARCH_SHARED_PATHS
#define QTPFS_SMOOTH_PATHS
// #define QTPFS_CONSERVATIVE_NODE_SPLITS
// #define QTPFS_DEBUG_NODE_HEAP
#define QTPFS_CORNER_CONNECTED_NODES
// #define QTPFS_SLOW_ACCURATE_TESSELATION
// #define QTPFS_OPENMP_ENABLED
// #define QTPFS_ORTHOPROJECTED_EDGE_TRANSITIONS
#define QTPFS_STAGGERED_LAYER_UPDATES
//
// #define QTPFS_VIRTUAL_NODE_FUNCTIONS
// #define QTPFS_ENABLE_THREADED_UPDATE
// #define QTPFS_AMORTIZED_NODE_NEIGHBOR_CACHE_UPDATES
#define QTPFS_ENABLE_MICRO_OPTIMIZATION_HACKS
// #define QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES

#define QTPFS_MAX_SMOOTHING_ITERATIONS 8

#define QTPFS_MAX_NETPOINTS_PER_NODE_EDGE 3
#define QTPFS_NETPOINT_EDGE_SPACING_SCALE (1.0f / (QTPFS_MAX_NETPOINTS_PER_NODE_EDGE + 1))

#define QTPFS_CACHE_VERSION 13
#define QTPFS_CACHE_XACCESS

#define QTPFS_POSITIVE_INFINITY (std::numeric_limits<float>::infinity())
#define QTPFS_CLOSED_NODE_COST (1 << 24)

#endif

