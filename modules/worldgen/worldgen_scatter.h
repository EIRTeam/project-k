#ifndef WORLDGEN_SCATTER_H
#define WORLDGEN_SCATTER_H

#include "core/io/resource.h"
#include "core/math/quaternion.h"
#include "core/math/random_pcg.h"
#include "core/math/transform_3d.h"
#include "core/object/ref_counted.h"
#include "core/variant/typed_array.h"
#include "scene/resources/packed_scene.h"
#include "worldgen/poisson_disk_sampling.h"
#include "worldgen/thirdparty/taskflow/core/executor.hpp"
#include "worldgen/thirdparty/taskflow/core/taskflow.hpp"

class WorldgenHeight;

class WorldgenScatterScene : public Resource {
    GDCLASS(WorldgenScatterScene, Resource);
    Ref<PackedScene> scene;
    float weight = 1.0f;

protected:
    static void _bind_methods();

public:
    Ref<PackedScene> get_scene() const { return scene; }
    void set_scene(const Ref<PackedScene> &p_scene) { scene = p_scene; }

    float get_weight() const { return weight; }
    void set_weight(float p_weight) { weight = p_weight; }
};

class WorldgenScatterLayer : public Resource {
    GDCLASS(WorldgenScatterLayer, Resource);

    float element_radius = 1.0f;
    float chance = 1.0f;
    float max_floor_angle = 45.0f;
    int min_lod = 5;

    Vector<Ref<WorldgenScatterScene>> scenes;

    TypedArray<WorldgenScatterScene> get_scenes_bind() const {
        TypedArray<WorldgenScatterScene> arr;

        for (Ref<WorldgenScatterScene> scene : scenes) {
            arr.push_back(scene);
        }

        return arr;
    }

    void set_scenes(const TypedArray<WorldgenScatterScene> &p_scenes) {
        scenes.clear();
        for (int i = 0; i < p_scenes.size(); i++) {
            scenes.push_back(p_scenes[i]);
        }
    }

    static void _bind_methods();

public:
    Vector<Ref<WorldgenScatterScene>> get_scenes() const {
        return scenes;
    }
    float get_element_radius() const { return element_radius; }
    void set_element_radius(float p_element_radius) { element_radius = p_element_radius; }

    float get_chance() const { return chance; }
    void set_chance(float p_chance) { chance = p_chance; }

    float get_max_floor_angle() const { return max_floor_angle; }
    void set_max_floor_angle(float p_max_floor_angle) { max_floor_angle = p_max_floor_angle; }

    int get_min_lod() const { return min_lod; }
    void set_min_lod(int p_min_lod) { min_lod = p_min_lod; }
};

class WorldgenScatterSettings : public Resource {
    GDCLASS(WorldgenScatterSettings, Resource);
    Vector<Ref<WorldgenScatterLayer>> layers;
public:
    Vector<Ref<WorldgenScatterLayer>> get_layers() const {
        return layers;
    }
    TypedArray<WorldgenScatterLayer> get_layers_bind() const {
        TypedArray<WorldgenScatterLayer> arr;

        for (Ref<WorldgenScatterLayer> layer : layers) {
            arr.push_back(layer);
        }

        return arr;
    }

    void set_layers(const TypedArray<WorldgenScatterLayer> &p_scenes) {
        layers.clear();
        for (int i = 0; i < p_scenes.size(); i++) {
            layers.push_back(p_scenes[i]);
        }
    }
protected:
    static void _bind_methods();
};

class WorldgenScatter : public Resource {
    GDCLASS(WorldgenScatter, Resource);
    Ref<WorldgenScatterSettings> settings;

protected:
    static void _bind_methods();
public:
    Ref<WorldgenScatterSettings> get_settings() const { return settings; }
    void set_settings(const Ref<WorldgenScatterSettings> &p_settings) { settings = p_settings; }

    struct ScatterSettings {
        int lod = 0;
        Rect2 region;
        uint64_t seed;
    };

    struct ScatterInstance {
        Transform3D transform;
        Ref<WorldgenScatterScene> scene;
    };

    void get_layer_weights_and_scenes(const Ref<WorldgenScatterLayer> &p_layer, Vector<float> &r_weights, Vector<Ref<WorldgenScatterScene>> &r_scenes) const {
        for (Ref<WorldgenScatterScene> scene : p_layer->get_scenes()) {
            if (scene.is_valid() && scene->get_scene().is_valid()) {
                r_weights.push_back(scene->get_weight());
                r_scenes.push_back(scene);
            }
        }
    }


    Vector<ScatterInstance> run_scatter(const ScatterSettings &p_settings, const WorldgenHeight *p_height_provider);
};

#endif // WORLDGEN_SCATTER_H
