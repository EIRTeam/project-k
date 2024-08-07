#ifndef CHUNKER_H
#define CHUNKER_H

#include "core/error/error_macros.h"
#include "core/math/rect2i.h"
#include "core/object/ref_counted.h"
#include "core/string/print_string.h"
#include "core/templates/local_vector.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"
#include "scene/resources/curve.h"
#include "scene/resources/mesh.h"
#include <iterator>
#include <array>

class ChunkerQuadTreeSettings : public Resource {
    GDCLASS(ChunkerQuadTreeSettings, Resource);

    Ref<Curve> lod_curve;
    int max_lods = 7;
    Rect2 bounds = Rect2(0, 0, 4096, 4096);

protected:
    static void _bind_methods();

public:
    Ref<Curve> get_lod_curve() const;
    void set_lod_curve(const Ref<Curve> &p_lod_curve);

    int get_max_lods() const;
    void set_max_lods(int p_max_lods);

    Rect2 get_bounds() const;
    void set_bounds(const Rect2 &p_bounds);
};

class ChunkerQuadTree : public RefCounted {
    GDCLASS(ChunkerQuadTree, RefCounted);
public:
    enum QuadTreeDirection {
        DIR_N,
        DIR_S,
        DIR_E,
        DIR_W,
        DIRECTION_MAX
    };

    enum QuadTreeChildPosition {
        POS_NW,
        POS_NE,
        POS_SE,
        POS_SW,
        POSITION_MAX
    };

    static constexpr QuadTreeDirection opposite_directions[4] = {
        DIR_S,
        DIR_N,
        DIR_W,
        DIR_E
    };

    static_assert(std::size(opposite_directions) == DIRECTION_MAX);
    
    static constexpr QuadTreeChildPosition positions_in_direction[4][2] = {
        {POS_NW, POS_NE},
        {POS_SW, POS_SE},
        {POS_SE, POS_NE},
        {POS_SW, POS_NW},
    };

private:
    class ChunkerQuadTreeNode : public RefCounted {
        GDCLASS(ChunkerQuadTreeNode, RefCounted);

        ChunkerQuadTreeNode* parent = nullptr;
        LocalVector<Ref<ChunkerQuadTreeNode>> children;
        int lod_level;
        Rect2 bounds;


        bool is_leaf() const {
            return children.size() == 0;
        }

        friend class ChunkerQuadTree;
    };

    Ref<ChunkerQuadTreeNode> root;
private:
    Ref<ChunkerQuadTreeSettings> settings;

    Ref<ChunkerQuadTreeNode> get_greater_or_equal_neighbor(const Ref<ChunkerQuadTreeNode> &p_node, QuadTreeDirection p_dir) const {
        if (p_node->parent == nullptr) {
            return Ref<ChunkerQuadTreeNode>();
        }

        const QuadTreeChildPosition *positions = positions_in_direction[p_dir];
        const QuadTreeChildPosition *opposite_positions = positions_in_direction[opposite_directions[p_dir]];

        for (int i = 0; i < 2; i++) {
            if (p_node->parent->children[opposite_positions[i]] == p_node) {
                return p_node->parent->children[positions[i]];
            }
        }

        Ref<ChunkerQuadTreeNode> neighbor_node = get_greater_or_equal_neighbor(p_node->parent, p_dir);

        if (neighbor_node.is_null() || neighbor_node->is_leaf()) {
            return neighbor_node;
        }

        return p_node->parent->children[positions[0]] == p_node ? neighbor_node->children[opposite_positions[0]] : neighbor_node->children[opposite_positions[1]];
    }

    Vector<Ref<ChunkerQuadTreeNode>> get_local_neighbors(const Ref<ChunkerQuadTreeNode> &p_neighbor, QuadTreeDirection p_dir) const {
        Vector<Ref<ChunkerQuadTreeNode>> local_neighbors;
        List<Ref<ChunkerQuadTreeNode>> candidates;

        candidates.push_back(p_neighbor);

        const QuadTreeChildPosition *positions = positions_in_direction[p_dir];

        while(!candidates.is_empty()) {
            Ref<ChunkerQuadTreeNode> candidate = candidates.front()->get();
            candidates.pop_front();

            if (candidate->is_leaf()) {
                local_neighbors.push_back(candidate);
            } else {
                for (int i = 0; i < 2; i++) {
                    candidates.push_back(candidate->children[positions[i]]);
                }
            }
        }

        return local_neighbors;
    }

    struct NeighborInfo {
        QuadTreeDirection direction;
        Ref<ChunkerQuadTreeNode> node;
    };
    
    Vector<NeighborInfo> get_neighbors(const Ref<ChunkerQuadTreeNode> &p_node) const {
        Vector<NeighborInfo> out;

        for(int i = 0; i < QuadTreeDirection::DIRECTION_MAX; i++) {
            Ref<ChunkerQuadTreeNode> greater_neighbor = get_greater_or_equal_neighbor(p_node, (QuadTreeDirection)i);
            
            if (!greater_neighbor.is_valid()) {
                continue;
            }

            Vector<Ref<ChunkerQuadTreeNode>> local_neighbors = get_local_neighbors(greater_neighbor, opposite_directions[i]);
            
            for (Ref<ChunkerQuadTreeNode> local_neighbor : local_neighbors) {
                NeighborInfo info;
                info.node = local_neighbor;
                info.direction = (QuadTreeDirection)i;
                out.push_back(info);
            }
        }

        return out;
    }

protected:
    static void _bind_methods();

public:
    ChunkerQuadTree() {
        settings.instantiate();
        clear();
    }

    void clear() {
        root.unref();
        root.instantiate();
        root->lod_level = 0;
        root->bounds = settings->get_bounds();
    }

    bool can_subdivide_node(const Ref<ChunkerQuadTreeNode> &p_node) const {
        return p_node->lod_level < settings->get_max_lods();
    }

    void subdivide_node(const Ref<ChunkerQuadTreeNode> &p_node) {
        ERR_FAIL_COND_MSG(p_node->children.size() != 0, "Tried to divide already subdivided node");
        ERR_FAIL_COND_MSG(!can_subdivide_node(p_node), "Tried to subdivide node that cannot be subdivided");
        
        Vector2 half_size = p_node->bounds.size / 2.0f;
        
        Rect2 rects[4] = {
            Rect2(p_node->bounds.position, half_size),
            Rect2(p_node->bounds.position + Vector2(half_size.x, 0), half_size),
            Rect2(p_node->bounds.position + half_size, half_size),
            Rect2(p_node->bounds.position + Vector2(0, half_size.y), half_size),
        };

        for (Rect2 rect : rects) {
            Ref<ChunkerQuadTreeNode> child_node;
            child_node.instantiate();
            child_node->lod_level = p_node->lod_level+1;
            child_node->parent = p_node.ptr();
            child_node->bounds = rect;
            p_node->children.push_back(child_node);
        }
    }

    bool should_subdivide_from_lod(const Ref<ChunkerQuadTreeNode> &p_node, const Vector2 &p_pos) const {
        ERR_FAIL_COND_V_EDMSG(!settings->get_lod_curve().is_valid(), false, "Didn't have an LOD curve");
        float lod_distance = 1.0 - p_node->lod_level/(float)(settings->get_max_lods()-1);
        lod_distance = (settings->get_bounds().size.x*0.5f) * settings->get_lod_curve()->sample(lod_distance);
        return can_subdivide_node(p_node) && Rect2(p_node->bounds).get_center().distance_to(p_pos) <= lod_distance;
    }

    void insert_camera(const Vector2 &p_point) {
        List<Ref<ChunkerQuadTreeNode>> candidates;
        candidates.push_back(root);
        
        while(candidates.size() > 0) {
            Ref<ChunkerQuadTreeNode> node = candidates.front()->get();
            candidates.pop_front();
            if (!should_subdivide_from_lod(node, p_point)) {
                continue;
            }
            subdivide_node(node);

            for (uint32_t i = 0; i < node->children.size(); i++) {
                Ref<ChunkerQuadTreeNode> candidate = node->children[i];
                candidates.push_back(candidate);
            }
        }

        balance_tree(root);
    }

    TypedArray<Dictionary> get_leaf_nodes_bind() const {
        List<Ref<ChunkerQuadTreeNode>> candidates;
        candidates.push_back(root);

        TypedArray<Dictionary> leaves; 

        while(candidates.size() > 0) {
            Ref<ChunkerQuadTreeNode> node = candidates.front()->get();
            candidates.pop_front();
            if (node->is_leaf()) {
                Dictionary leaf_dict;
                leaf_dict["bounds"] = node->bounds;
                leaf_dict["lod_level"] = node->lod_level;
                leaves.push_back(leaf_dict);
            }

            for (const Ref<ChunkerQuadTreeNode> &child : node->children) {
                candidates.push_back(child);
            }
        }

        return leaves;
    }

    void balance_tree(const Ref<ChunkerQuadTreeNode> &p_node) {
        if (p_node->is_leaf()) {
            Vector<NeighborInfo> neighbors = get_neighbors(p_node);
            for (const NeighborInfo &neighbor : neighbors) {
                if (Math::abs(neighbor.node->lod_level - p_node->lod_level) <= 1) {
                    continue;
                }

                Ref<ChunkerQuadTreeNode> lowest = p_node->lod_level >= neighbor.node->lod_level ? neighbor.node : p_node;

                if (can_subdivide_node(lowest)) {
                    subdivide_node(lowest);
                    balance_tree(lowest);
                    break;
                }
            }
        }

        for (Ref<ChunkerQuadTreeNode> child : p_node->children) {
            balance_tree(child);
        }
    }

    Ref<ChunkerQuadTreeNode> intersect(const Vector2i &p_point) const {
        if (!root->bounds.has_point(p_point)) {
            return Ref<ChunkerQuadTreeNode>();
        }

        Ref<ChunkerQuadTreeNode> node = root;

        while (!node->is_leaf()) {
            bool found_node = false;
            for (const Ref<ChunkerQuadTreeNode> &child : node->children) {
                if (child->bounds.has_point(p_point)) {
                    node = child;
                    found_node = true;
                    break;
                }
            }

            if (!found_node) {
                ERR_PRINT_ONCE_ED("Chunker intersect bug?");
                break;
            }
        }

        return node;
    }

    TypedArray<Dictionary> get_neighbors_at(const Vector2i &p_point) {
        TypedArray<Dictionary> out;

        Ref<ChunkerQuadTreeNode> node = intersect(p_point);

        if (node.is_valid()) {
            Vector<NeighborInfo> local_neighbors = get_neighbors(node);

            for (const NeighborInfo &neighbor : local_neighbors) {
                Dictionary dict;
                dict["bounds"] = neighbor.node->bounds;
                out.push_back(dict);
            }
        }

        return out;
    }

    typedef std::array<int, 4> NeighborLODs;

    struct LeafNodeInfo {
        Rect2 bounds;
        int lod_level;
        NeighborLODs neighbor_lods = {-1, -1, -1, -1};
    };

    Vector<LeafNodeInfo> get_leaf_node_infos() const {
        List<Ref<ChunkerQuadTreeNode>> candidates;
        candidates.push_back(root);

        Vector<LeafNodeInfo> leaves; 

        while(candidates.size() > 0) {
            Ref<ChunkerQuadTreeNode> node = candidates.front()->get();
            candidates.pop_front();
            if (node->is_leaf()) {

                LeafNodeInfo node_info;
                node_info.bounds = node->bounds;
                node_info.lod_level = node->lod_level;

                Vector<NeighborInfo> neighbors = get_neighbors(node);

                for (NeighborInfo info : neighbors) {
                    node_info.neighbor_lods[info.direction] = info.node->lod_level;
                }
                leaves.push_back(node_info);
            }

            for (const Ref<ChunkerQuadTreeNode> &child : node->children) {
                candidates.push_back(child);
            }
        }

        return leaves;
    }

    TypedArray<Dictionary> get_leaf_node_infos_bind() const {
        TypedArray<Dictionary> out;

        for (const LeafNodeInfo &info : get_leaf_node_infos()) {
            Dictionary dict;

            dict["bounds"] = info.bounds;
            dict["lod"] = info.lod_level;

            TypedArray<int> lods;
            lods.push_back(info.neighbor_lods[0]);
            lods.push_back(info.neighbor_lods[1]);
            lods.push_back(info.neighbor_lods[2]);
            lods.push_back(info.neighbor_lods[3]);
            
            dict["neighbor_lods"] = lods;
            out.push_back(dict);
        }

        return out;
    }

    static Ref<ChunkerQuadTree> create_bind() {
        Ref<ChunkerQuadTree> node;
        node.instantiate();
        return node;
    }

    Rect2 get_bounds() const {
        return settings->get_bounds();
    }

    static Ref<ArrayMesh> build_mesh(int p_element_count, float p_side_length, int p_mask);

    Ref<ChunkerQuadTreeSettings> get_settings() const {
        return settings;
    }

    void set_settings(const Ref<ChunkerQuadTreeSettings> &p_settings) {
        ERR_FAIL_COND(!p_settings.is_valid());
        if (settings.is_valid()) {
            settings->disconnect("changed", callable_mp(this, &ChunkerQuadTree::clear));
        }
        settings = p_settings;
        settings->connect("changed", callable_mp(this, &ChunkerQuadTree::clear));
    }

    float get_lod_distance_threshold(int p_lod_level) const {
        ERR_FAIL_COND_V_EDMSG(!settings->get_lod_curve().is_valid(), false, "Didn't have an LOD curve");
        float lod_distance = 1.0 - p_lod_level/(float)(settings->get_max_lods()-1);
        lod_distance = (settings->get_bounds().size.x*0.5f) * settings->get_lod_curve()->sample(lod_distance);
        return lod_distance;
    }
};

#endif // CHUNKER_H