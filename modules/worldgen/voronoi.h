#ifndef VORONOI_H
#define VORONOI_H

#include "core/error/error_macros.h"
#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"
#include "core/variant/variant.h"
#include "thirdparty/voronoi/jc_voronoi.h"
#include <cstddef>

class VoronoiGraph : public RefCounted {
    GDCLASS(VoronoiGraph, RefCounted);
    jcv_diagram diagram = {};
    jcv_delauney_iter delaunay_iter = {};
    struct SiteTriangle {
        int other_sites[2];
        jcv_delauney_edge edges[3];
    };
    LocalVector<LocalVector<SiteTriangle>> site_triangles;
    // For some heaven forsaken reason jcv's diagrams don't store site points in order of arrival
    PackedVector2Array site_positions;
protected:
    static void _bind_methods();
public:
    static Ref<VoronoiGraph> create(PackedVector2Array p_sites);

    int get_site_count() const;

    PackedVector2Array get_site_edges(int p_site) const;
    TypedArray<PackedInt32Array> get_site_triangles(int p_site) const;
    Vector2 get_site_position(int p_site) const;
    void delaunay_iter_begin();
    Dictionary delaunay_iter_next();


    ~VoronoiGraph();
};

#endif // VORONOI_H
