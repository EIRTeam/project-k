#ifndef QUADTREE_LAYER_H
#define QUADTREE_LAYER_H

#include "core/math/rect2.h"
#include "core/string/print_string.h"
#include "layer_manager.h"
#include "../thirdparty/taskflow/core/taskflow.hpp"
#include "../plane_generate.h"
#include "scene/3d/mesh_instance_3d.h"
#include "worldgen/chunker.h"
#include "worldgen/instance_texture_queue.h"
#include "worldgen/layer_system/road_layer.h"

class QuadTreeTerrainChunk;
class HeightmapLayer;

class QuadTreeTerrainLayer : public ChunkerLayer {
    GDCLASS(QuadTreeTerrainLayer, ChunkerLayer);
    HashMap<BitField<PlaneGenerate::GridTJunctionRemovalFlags>, Ref<Mesh>> grid_meshes;
    Ref<ChunkerQuadTreeSettings> quad_tree_settings;
    Vector2 camera_position;
    Ref<Material> terrain_material;
    Ref<RoadLayer> road_layer;
public:
    QuadTreeTerrainLayer(Ref<RoadLayer> p_road_layer);
    virtual float get_chunk_size() const override {
        return 2048.0f;
    }
    virtual Ref<ChunkerChunk> create_chunk() const  override;
    Vector2 get_camera_position() const;
    void set_camera_position(const Vector2 &p_camera_position);
    void update_terrain_chunks();
    friend class QuadTreeTerrainChunk;
};

class QuadTreeTerrainChunk : public ChunkerChunk {
    GDCLASS(QuadTreeTerrainChunk, ChunkerChunk);
    QuadTreeTerrainLayer *layer = nullptr;
    Ref<ChunkerQuadTree> quad_tree;
    Vector2 camera_position;
    Node3D *chunk_node = nullptr;

    struct GridNode {
        MeshInstance3D *mi = nullptr;
        int lod_level;
        BitField<PlaneGenerate::GridTJunctionRemovalFlags> lod_flags;
    };

    HashMap<Rect2, GridNode> loaded_grid_nodes;
public:
    QuadTreeTerrainChunk(QuadTreeTerrainLayer *p_layer);
    void update_quadtree();
    void update_mesh_instances();
    virtual void build(tf::Taskflow &p_taskflow) override;

    static BitField<PlaneGenerate::GridTJunctionRemovalFlags> get_tjunction_mesh_flags(int p_lod, ChunkerQuadTree::NeighborLODs p_lods) {
        BitField<PlaneGenerate::GridTJunctionRemovalFlags> flags;

        if (p_lods[ChunkerQuadTree::DIR_N] != -1 && p_lods[ChunkerQuadTree::DIR_N] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::UP);
        }
        if (p_lods[ChunkerQuadTree::DIR_S] != -1 && p_lods[ChunkerQuadTree::DIR_S] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::DOWN);
        }
        if (p_lods[ChunkerQuadTree::DIR_E] != -1 && p_lods[ChunkerQuadTree::DIR_E] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::RIGHT);
        }
        if (p_lods[ChunkerQuadTree::DIR_W] != -1 && p_lods[ChunkerQuadTree::DIR_W] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::LEFT);
        }
        return flags;
    }

    Ref<Mesh> get_mesh_for_lods(int p_lod, ChunkerQuadTree::NeighborLODs p_neighbor_lods) {
        return layer->grid_meshes[get_tjunction_mesh_flags(p_lod, p_neighbor_lods)];
    }

    void unload_grid_node(const GridNode &p_grid_node) {
        p_grid_node.mi->queue_free();
    }

    virtual void unload() override;

    virtual void on_build_completed() override;
    friend class QuadTreeTerrainLayer;
};

#endif // QUADTREE_LAYER_H
