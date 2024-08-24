#include "chunker.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "plane_generate.h"
#include "scene/resources/mesh.h"

Ref<ChunkerQuadTree::ChunkerQuadTreeNode> ChunkerQuadTree::get_greater_or_equal_neighbor(const Ref<ChunkerQuadTreeNode> &p_node, QuadTreeDirection p_dir) const {
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

Vector<Ref<ChunkerQuadTree::ChunkerQuadTreeNode>> ChunkerQuadTree::get_local_neighbors(const Ref<ChunkerQuadTreeNode> &p_neighbor, QuadTreeDirection p_dir) const {
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

Vector<ChunkerQuadTree::NeighborInfo> ChunkerQuadTree::get_neighbors(const Ref<ChunkerQuadTreeNode> &p_node) const {
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

void ChunkerQuadTree::_bind_methods() {
    ClassDB::bind_method(D_METHOD("insert_camera", "position"), &ChunkerQuadTree::insert_camera);
    ClassDB::bind_method(D_METHOD("get_leaf_nodes"), &ChunkerQuadTree::get_leaf_nodes_bind);
    ClassDB::bind_method(D_METHOD("get_neighbors_at", "point"), &ChunkerQuadTree::get_neighbors_at);
    ClassDB::bind_method(D_METHOD("get_leaf_node_infos"), &ChunkerQuadTree::get_leaf_node_infos_bind);
    ClassDB::bind_static_method("ChunkerQuadTree", D_METHOD("create"), &ChunkerQuadTree::create_bind);
    ClassDB::bind_static_method("ChunkerQuadTree", D_METHOD("build_mesh", "element_count", "side_length", "mask"), &ChunkerQuadTree::build_mesh);
}

ChunkerQuadTree::ChunkerQuadTree() {
    settings.instantiate();
    clear();
}

void ChunkerQuadTree::clear() {
    root.unref();
    root.instantiate();
    root->lod_level = 0;
    root->bounds = bounds;
}

bool ChunkerQuadTree::can_subdivide_node(const Ref<ChunkerQuadTreeNode> &p_node) const {
    return p_node->lod_level < settings->get_max_lods();
}

void ChunkerQuadTree::subdivide_node(const Ref<ChunkerQuadTreeNode> &p_node) {
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

bool ChunkerQuadTree::should_subdivide_from_lod(const Ref<ChunkerQuadTreeNode> &p_node, const Vector2 &p_pos) const {
    float lod_distance = (Math_SQRT2 * bounds.size.x) / Math::pow(2.0f, p_node->lod_level);
    return can_subdivide_node(p_node) && Rect2(p_node->bounds).get_center().distance_to(p_pos) <= lod_distance;
}

void ChunkerQuadTree::insert_camera(const Vector2 &p_point) {
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

void ChunkerQuadTree::set_bounds(Rect2 &p_bounds) {
    bounds = p_bounds;
}

TypedArray<Dictionary> ChunkerQuadTree::get_leaf_nodes_bind() const {
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

void ChunkerQuadTree::balance_tree(const Ref<ChunkerQuadTreeNode> &p_node) {
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

Ref<ChunkerQuadTree::ChunkerQuadTreeNode> ChunkerQuadTree::intersect(const Vector2i &p_point) const {
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

TypedArray<Dictionary> ChunkerQuadTree::get_neighbors_at(const Vector2i &p_point) {
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

Vector<ChunkerQuadTree::LeafNodeInfo> ChunkerQuadTree::get_leaf_node_infos() const {
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

TypedArray<Dictionary> ChunkerQuadTree::get_leaf_node_infos_bind() const {
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

Ref<ChunkerQuadTree> ChunkerQuadTree::create_bind() {
    Ref<ChunkerQuadTree> node;
    node.instantiate();
    return node;
}

Rect2 ChunkerQuadTree::get_bounds() const {
    return bounds;
}

Ref<ArrayMesh> ChunkerQuadTree::build_mesh(int p_element_count, float p_side_length, int p_mask) {
    PlaneGenerate::GridMesh mesh;
    PlaneGenerate::generate_mesh({
        .element_count = p_element_count,
        .side_length = p_side_length
    }, p_mask, mesh);

    Array mesh_arr;
    mesh_arr.resize(RS::ARRAY_MAX);
    mesh_arr[RS::ARRAY_INDEX] = mesh.indices;
    mesh_arr[RS::ARRAY_VERTEX] = mesh.positions;
    mesh_arr[RS::ARRAY_TEX_UV] = mesh.uvs;

    Ref<ArrayMesh> mesh_godot;
    mesh_godot.instantiate();
    mesh_godot->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arr);
    return mesh_godot;
}

Ref<ChunkerQuadTreeSettings> ChunkerQuadTree::get_settings() const {
    return settings;
}

void ChunkerQuadTree::set_settings(const Ref<ChunkerQuadTreeSettings> &p_settings) {
    ERR_FAIL_COND(!p_settings.is_valid());
    settings = p_settings;
}

float ChunkerQuadTree::get_lod_distance_threshold(int p_lod_level) const {
    float lod_distance = (Math_SQRT2 * bounds.size.x) / Math::pow(2.0f, p_lod_level);
    return lod_distance;
}

float ChunkerQuadTree::get_lod_side_size(int p_lod_level) const {
    return bounds.size.x / Math::pow(2.0f, p_lod_level);
}

void ChunkerQuadTreeSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_max_lods", "max_lods"), &ChunkerQuadTreeSettings::set_max_lods);
    ClassDB::bind_method(D_METHOD("get_max_lods"), &ChunkerQuadTreeSettings::get_max_lods);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lods", PROPERTY_HINT_RANGE, "1,25,1"), "set_max_lods", "get_max_lods");
}

int ChunkerQuadTreeSettings::get_max_lods() const {
    return max_lods;
}

void ChunkerQuadTreeSettings::set_max_lods(int p_max_lods) {
    ERR_FAIL_COND(p_max_lods < 0);
    max_lods = p_max_lods;
    emit_changed();
}

bool ChunkerQuadTree::ChunkerQuadTreeNode::is_leaf() const {
    return children.size() == 0;
}
