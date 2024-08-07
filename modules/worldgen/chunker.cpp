#include "chunker.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "plane_generate.h"
#include "scene/resources/mesh.h"

void ChunkerQuadTree::_bind_methods() {
    ClassDB::bind_method(D_METHOD("insert_camera", "position"), &ChunkerQuadTree::insert_camera);
    ClassDB::bind_method(D_METHOD("get_leaf_nodes"), &ChunkerQuadTree::get_leaf_nodes_bind);
    ClassDB::bind_method(D_METHOD("get_neighbors_at", "point"), &ChunkerQuadTree::get_neighbors_at);
    ClassDB::bind_method(D_METHOD("get_leaf_node_infos"), &ChunkerQuadTree::get_leaf_node_infos_bind);
    ClassDB::bind_static_method("ChunkerQuadTree", D_METHOD("create"), &ChunkerQuadTree::create_bind);
    ClassDB::bind_static_method("ChunkerQuadTree", D_METHOD("build_mesh", "element_count", "side_length", "mask"), &ChunkerQuadTree::build_mesh);
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

void ChunkerQuadTreeSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_lod_curve", "lod_curve"), &ChunkerQuadTreeSettings::set_lod_curve);
    ClassDB::bind_method(D_METHOD("get_lod_curve"), &ChunkerQuadTreeSettings::get_lod_curve);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "lod_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_lod_curve", "get_lod_curve");

    ClassDB::bind_method(D_METHOD("set_max_lods", "max_lods"), &ChunkerQuadTreeSettings::set_max_lods);
    ClassDB::bind_method(D_METHOD("get_max_lods"), &ChunkerQuadTreeSettings::get_max_lods);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_lods", PROPERTY_HINT_RANGE, "1,25,1"), "set_max_lods", "get_max_lods");

    ClassDB::bind_method(D_METHOD("set_bounds", "max_lods"), &ChunkerQuadTreeSettings::set_bounds);
    ClassDB::bind_method(D_METHOD("get_bounds"), &ChunkerQuadTreeSettings::get_bounds);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "bounds"), "set_bounds", "get_bounds");

}
Ref<Curve> ChunkerQuadTreeSettings::get_lod_curve() const {
    return lod_curve;
}
void ChunkerQuadTreeSettings::set_lod_curve(const Ref<Curve> &p_lod_curve) {
    if (lod_curve.is_valid()) {
        lod_curve->disconnect("changed", callable_mp((Resource*)this, &ChunkerQuadTreeSettings::emit_changed));
    }
    
    lod_curve = p_lod_curve;
    
    if (lod_curve.is_valid()) {
        lod_curve->connect("changed", callable_mp((Resource*)this, &ChunkerQuadTreeSettings::emit_changed));
    }

    emit_changed();
}

int ChunkerQuadTreeSettings::get_max_lods() const {
    return max_lods;
}

void ChunkerQuadTreeSettings::set_max_lods(int p_max_lods) {
    ERR_FAIL_COND(p_max_lods < 0);
    max_lods = p_max_lods;
    emit_changed();
}

Rect2 ChunkerQuadTreeSettings::get_bounds() const { return bounds; }

void ChunkerQuadTreeSettings::set_bounds(const Rect2 &p_bounds) {
    ERR_FAIL_COND(!p_bounds.has_area());
    bounds = p_bounds;
    emit_changed();
}
