#ifndef QUADTREE_H
#define QUADTREE_H

#include "core/math/plane.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/variant/variant.h"
#include "servers/rendering/renderer_scene_cull.h"
#include <vector>

// Strategy (for now) is to keep a completely subdivided quad tree in memory, it might be too
// memory intensive, so perhaps we could change it later
class QuadTreeCull : public RefCounted {
    GDCLASS(QuadTreeCull, RefCounted);
public:
    struct QuadTreeSettings {
        int max_depth = 8;
        int chunk_size = 32;
        float chunk_height = 50;
    };
private:
    QuadTreeSettings settings;

    struct QuadTreeNode {
        int node_size;
        bool leaf = false;
        Vector3 world_position;
        RendererSceneCull::InstanceBounds bounds = {};
        LocalVector<QuadTreeNode> children = {};
    };

    QuadTreeNode root;

    void _subdiv_tree_impl(QuadTreeNode *p_node) {
        int half_node_size = p_node->node_size / 2;

        p_node->bounds = RendererSceneCull::InstanceBounds(AABB(p_node->world_position, Vector3(p_node->node_size, settings.chunk_height, p_node->node_size)));
        if (!p_node->leaf) {
            Vector3 world_positions[4] = {
                p_node->world_position + Vector3(0, 0, 0),
                p_node->world_position + Vector3(half_node_size, 0, 0),
                p_node->world_position + Vector3(half_node_size, 0, half_node_size),
                p_node->world_position + Vector3(0, 0, half_node_size),
            };

            p_node->children.reserve(4);
            for (int i = 0; i < 4; i++) {
                QuadTreeNode node {
                    .node_size = half_node_size,
                    .leaf = half_node_size == settings.chunk_size,
                    .world_position = world_positions[i]
                };
                p_node->children.push_back(node);
                _subdiv_tree_impl(&p_node->children.ptr()[i]);
            }
        }
    }

    bool is_node_in_frustum(const RendererSceneCull::Frustum &p_frustum, const QuadTreeNode &p_node) {
        return p_node.bounds.in_frustum(p_frustum);
    }

    void _intersect_frustum_impl(const QuadTreeNode &p_node, const RendererSceneCull::Frustum &p_frustum, std::vector<Vector3> &r_out) {
        if (is_node_in_frustum(p_frustum, p_node)) {
            if (!p_node.leaf) {
                for (const QuadTreeNode &node : p_node.children) {
                    _intersect_frustum_impl(node, p_frustum, r_out);
                }
            } else {
                r_out.push_back(p_node.world_position);
            }
        }
    }

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("intersect_frustum", "planes"), &QuadTreeCull::intersect_frustum_bind);
    }
public:
    std::vector<Vector3> intersect_frustum(const Vector<Plane> p_projection_planes) {
        RendererSceneCull::Frustum frustum = RendererSceneCull::Frustum(p_projection_planes);
        std::vector<Vector3> out;
        _intersect_frustum_impl(root, frustum, out);
        return out;
    }

    PackedVector3Array intersect_frustum_bind(TypedArray<Plane> p_planes) {
        Vector<Plane> planes;
        for (int i = 0; i < p_planes.size(); i++) {
            planes.push_back(p_planes[i]);
        }
        std::vector<Vector3> std_out = intersect_frustum(planes);

        PackedVector3Array out;
        out.resize(std_out.size());

        for(int i = 0; i < out.size(); i++) {
            out.ptrw()[i] = std_out[i];
        }

        return out;
    }


    void subdiv_to_max_depth() {
        root.node_size = settings.chunk_size * settings.max_depth;
        root.world_position = Vector3();
        _subdiv_tree_impl(&root);
    }

    QuadTreeCull(const QuadTreeSettings &p_settings) {
        settings = p_settings;
    }
};

#endif // QUADTREE_H
