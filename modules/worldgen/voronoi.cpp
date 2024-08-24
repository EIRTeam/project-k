#include "voronoi.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"
#include "core/templates/pair.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

#define JC_VORONOI_IMPLEMENTATION
#include "thirdparty/voronoi/jc_voronoi.h"

void VoronoiGraph::_bind_methods() {
    ClassDB::bind_static_method("VoronoiGraph", D_METHOD("create", "sites"), &VoronoiGraph::create);
    ClassDB::bind_method(D_METHOD("get_site_count"), &VoronoiGraph::get_site_count);
    ClassDB::bind_method(D_METHOD("get_site_edges", "site"), &VoronoiGraph::get_site_edges);
    ClassDB::bind_method(D_METHOD("delaunay_iter_begin"), &VoronoiGraph::delaunay_iter_begin);
    ClassDB::bind_method(D_METHOD("delaunay_iter_next"), &VoronoiGraph::delaunay_iter_next);
    ClassDB::bind_method(D_METHOD("get_site_triangles", "site"), &VoronoiGraph::get_site_triangles);
    ClassDB::bind_method(D_METHOD("get_site_position", "site"), &VoronoiGraph::get_site_position);
}

Ref<VoronoiGraph> VoronoiGraph::create(PackedVector2Array p_sites) {
    Ref<VoronoiGraph> graph;
    graph.instantiate();

    graph->site_positions = p_sites;

    Vector<jcv_point> points;
    points.resize(p_sites.size());
    jcv_point *site_points = points.ptrw();
    for (int i = 0; i < p_sites.size(); i++) {
        site_points[i] = {
            .x = p_sites[i].x,
            .y = p_sites[i].y,
        };
    }

    jcv_diagram_generate(points.size(), site_points, nullptr, nullptr, &graph->diagram);
    
    // Calculate triangle list per site
    LocalVector<LocalVector<jcv_delauney_edge>> per_site_delaunay_edges;
    per_site_delaunay_edges.resize(points.size());
    LocalVector<HashMap<int, jcv_delauney_edge>> delaunay_edges_bucketed;
    delaunay_edges_bucketed.resize(points.size());

    jcv_delauney_iter iter;
    jcv_delauney_begin(&graph->diagram, &iter);

    jcv_delauney_edge edge;
    while (jcv_delauney_next(&iter, &edge) != 0) {
        per_site_delaunay_edges[edge.sites[0]->index].push_back(edge);
        per_site_delaunay_edges[edge.sites[1]->index].push_back(edge);
        delaunay_edges_bucketed[edge.sites[0]->index].insert(edge.sites[1]->index, edge);
        delaunay_edges_bucketed[edge.sites[1]->index].insert(edge.sites[0]->index, edge);
    }

    graph->site_triangles.resize(points.size());

    for (int i = 0; i < points.size(); i++) {
        for (size_t j = 0; j < per_site_delaunay_edges[i].size(); j++) {
            const jcv_delauney_edge &edge_1 = per_site_delaunay_edges[i][j];
            int other_site_1 = edge_1.sites[0]->index == i ? edge_1.sites[1]->index : edge_1.sites[0]->index;
            for (size_t k = j+1; k < per_site_delaunay_edges[i].size(); k++) {
                const jcv_delauney_edge &edge_2 = per_site_delaunay_edges[i][k];
                // we got the two edges that connect this site to other sites,
                // now try to find an edge between those to form a triangle
                int other_site_2 = edge_2.sites[0]->index == i ? edge_2.sites[1]->index : edge_2.sites[0]->index;
                HashMap<int, jcv_delauney_edge>::ConstIterator it = delaunay_edges_bucketed[other_site_1].find(other_site_2);
                if (it == delaunay_edges_bucketed[other_site_1].end()) {
                    continue;
                }

                graph->site_triangles[i].push_back({
                    .other_sites = {
                        other_site_1,
                        other_site_2
                    }
                });
            }
        }
    }
    
    return graph;
}

int VoronoiGraph::get_site_count() const {
    return diagram.numsites;
}

PackedVector2Array VoronoiGraph::get_site_edges(int p_site) const {
    ERR_FAIL_INDEX_V(p_site, diagram.numsites, PackedVector2Array());
    const jcv_site* sites = jcv_diagram_get_sites(&diagram);
    jcv_graphedge *edge = sites[p_site].edges;
    
    PackedVector2Array edges;

    while (edge) {
        edges.push_back(Vector2(edge->pos[0].x, edge->pos[0].y));
        edge = edge->next;
    }

    return edges;
}

TypedArray<PackedInt32Array> VoronoiGraph::get_site_triangles(int p_site) const {
    ERR_FAIL_INDEX_V(p_site, diagram.numsites, TypedArray<PackedInt32Array>());
    TypedArray<PackedInt32Array> out;

    for (size_t i = 0; i < site_triangles[p_site].size(); i++) {
        PackedInt32Array arr;
        arr.push_back(site_triangles[p_site][i].other_sites[0]);
        arr.push_back(site_triangles[p_site][i].other_sites[1]);
        out.push_back(arr);
    }
    return out;
}

Vector2 VoronoiGraph::get_site_position(int p_site) const {
    ERR_FAIL_INDEX_V(p_site, diagram.numsites, Vector2());
    return site_positions[p_site];
}

void VoronoiGraph::delaunay_iter_begin() {
    jcv_delauney_begin(&diagram, &delaunay_iter);
}

Dictionary VoronoiGraph::delaunay_iter_next() {
    jcv_delauney_edge edge;
    if (jcv_delauney_next(&delaunay_iter, &edge) == 0) {
        return Dictionary();
    }
    Dictionary dict;
    dict["pos_0"] = Vector2(edge.pos[0].x, edge.pos[0].y);
    dict["pos_1"] = Vector2(edge.pos[1].x, edge.pos[1].y);
    dict["site_0"] = edge.sites[0]->index;
    dict["site_1"] = edge.sites[1]->index;
    return dict;
}

VoronoiGraph::~VoronoiGraph() {
    jcv_diagram_free(&diagram);
}
