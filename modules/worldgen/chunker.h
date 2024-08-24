#ifndef CHUNKER_H
#define CHUNKER_H

#include "core/error/error_macros.h"
#include "core/math/math_defs.h"
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
    int max_lods = 5;

protected:
    static void _bind_methods();

public:
    Ref<Curve> get_lod_curve() const;
    void set_lod_curve(const Ref<Curve> &p_lod_curve);

    int get_max_lods() const;
    void set_max_lods(int p_max_lods);
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


        bool is_leaf() const;

        friend class ChunkerQuadTree;
    };

    Ref<ChunkerQuadTreeNode> root;
    Ref<ChunkerQuadTreeSettings> settings;
    Rect2 bounds;

    Ref<ChunkerQuadTreeNode> get_greater_or_equal_neighbor(const Ref<ChunkerQuadTreeNode> &p_node, QuadTreeDirection p_dir) const;

    Vector<Ref<ChunkerQuadTreeNode>> get_local_neighbors(const Ref<ChunkerQuadTreeNode> &p_neighbor, QuadTreeDirection p_dir) const;

    struct NeighborInfo {
        QuadTreeDirection direction;
        Ref<ChunkerQuadTreeNode> node;
    };
    
    Vector<NeighborInfo> get_neighbors(const Ref<ChunkerQuadTreeNode> &p_node) const;

protected:
    static void _bind_methods();

public:
    ChunkerQuadTree();

    void clear();

    bool can_subdivide_node(const Ref<ChunkerQuadTreeNode> &p_node) const;
    void subdivide_node(const Ref<ChunkerQuadTreeNode> &p_node);
    bool should_subdivide_from_lod(const Ref<ChunkerQuadTreeNode> &p_node, const Vector2 &p_pos) const;
    void insert_camera(const Vector2 &p_point);
    void set_bounds(Rect2 &p_bounds);
    TypedArray<Dictionary> get_leaf_nodes_bind() const;

    void balance_tree(const Ref<ChunkerQuadTreeNode> &p_node);

    Ref<ChunkerQuadTreeNode> intersect(const Vector2i &p_point) const;

    TypedArray<Dictionary> get_neighbors_at(const Vector2i &p_point);

    typedef std::array<int, 4> NeighborLODs;

    struct LeafNodeInfo {
        Rect2 bounds;
        int lod_level;
        NeighborLODs neighbor_lods = {-1, -1, -1, -1};
    };

    Vector<LeafNodeInfo> get_leaf_node_infos() const;

    TypedArray<Dictionary> get_leaf_node_infos_bind() const;

    static Ref<ChunkerQuadTree> create_bind();

    Rect2 get_bounds() const;

    static Ref<ArrayMesh> build_mesh(int p_element_count, float p_side_length, int p_mask);

    Ref<ChunkerQuadTreeSettings> get_settings() const;

    void set_settings(const Ref<ChunkerQuadTreeSettings> &p_settings);

    float get_lod_distance_threshold(int p_lod_level) const;

    float get_lod_side_size(int p_lod_level) const;
};

#endif // CHUNKER_H