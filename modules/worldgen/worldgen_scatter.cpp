#include "worldgen_scatter.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/string/print_string.h"
#include "worldgen_manager.h"

void WorldgenScatterLayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_scenes", "scenes"), &WorldgenScatterLayer::set_scenes);
    ClassDB::bind_method(D_METHOD("get_scenes"), &WorldgenScatterLayer::get_scenes_bind);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "scenes", PROPERTY_HINT_ARRAY_TYPE, "WorldgenScatterScene"), "set_scenes", "get_scenes");

    ClassDB::bind_method(D_METHOD("set_element_radius", "element_radius"), &WorldgenScatterLayer::set_element_radius);
    ClassDB::bind_method(D_METHOD("get_element_radius"), &WorldgenScatterLayer::get_element_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "element_radius"), "set_element_radius", "get_element_radius");

    ClassDB::bind_method(D_METHOD("set_chance", "chance"), &WorldgenScatterLayer::set_chance);
    ClassDB::bind_method(D_METHOD("get_chance"), &WorldgenScatterLayer::get_chance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chance"), "set_chance", "get_chance");

    ClassDB::bind_method(D_METHOD("set_max_floor_angle", "max_floor_angle"), &WorldgenScatterLayer::set_max_floor_angle);
    ClassDB::bind_method(D_METHOD("get_max_floor_angle"), &WorldgenScatterLayer::get_max_floor_angle);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_floor_angle", PROPERTY_HINT_RANGE, "-360,360,0.1,or_less,or_greater,radians_as_degrees"), "set_max_floor_angle", "get_max_floor_angle");

    ClassDB::bind_method(D_METHOD("set_min_lod", "min_lod"), &WorldgenScatterLayer::set_min_lod);
    ClassDB::bind_method(D_METHOD("get_min_lod"), &WorldgenScatterLayer::get_min_lod);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "min_lod", PROPERTY_HINT_RANGE, "0,10,1,or_greater"), "set_min_lod", "get_min_lod");

}

void WorldgenScatter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_settings", "settings"), &WorldgenScatter::set_settings);
    ClassDB::bind_method(D_METHOD("get_settings"), &WorldgenScatter::get_settings);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "settings", PROPERTY_HINT_RESOURCE_TYPE, "WorldgenScatterSettings"), "set_settings", "get_settings");
};

Vector<WorldgenScatter::ScatterInstance> WorldgenScatter::run_scatter(const ScatterSettings &p_settings, const WorldgenHeight *p_height_provider) {
    Vector<Ref<WorldgenScatterLayer>> layers = settings->get_layers();
    Vector<ScatterInstance> out;

    RandomPCG pcg = RandomPCG(p_settings.seed);

    for (int i = 0; i < layers.size(); i++) {
        const Ref<WorldgenScatterLayer> &layer = layers[i];

        if (layer->get_min_lod() > p_settings.lod) {
            continue;
        }

        Vector<float> weights;
        Vector<Ref<WorldgenScatterScene>> scenes;
        get_layer_weights_and_scenes(layer, weights, scenes);
        if (scenes.size() == 0) {
            continue;
        }

        // Reduce the placement area on two sides to ensure non overlapping placements
        Vector2 area_size = p_settings.region.size;
        area_size -= Vector2(layer->get_element_radius(), layer->get_element_radius());
        
        PoissonDiskSampler sampler = PoissonDiskSampler({
            .sample_area = area_size,
            .element_radius = layer->get_element_radius(),
            .seed = p_settings.seed,
        });

        LocalVector<PoissonDiskSampler::PoissonSampleCell> cells = sampler.generate();
        for (const PoissonDiskSampler::PoissonSampleCell &cell : cells) {
            const Vector2 pos = p_settings.region.position + cell.position;
            Vector3 normal;
            float height;
            p_height_provider->get_height_with_normal(pos, normal, height);

            const float ground_normal = Vector3(0.0f, 1.0f, 0.0f).angle_to(normal);
            if (ground_normal > layer->get_max_floor_angle()) {
                continue;
            }

            if (layer->get_chance() < pcg.randf()) {
                continue;
            }

            Transform3D trf;
            trf.basis.rotate(Vector3(0.0, 1.0, 0.0), cell.angle_rads);
            trf.basis = Quaternion(Vector3(0.0, 1.0, 0.0), normal) * trf.basis;
            trf.origin = Vector3(pos.x, height, pos.y);

            Ref<WorldgenScatterScene> scene = scenes[pcg.rand_weighted(weights)];

            out.push_back({
                .transform = trf,
                .scene = scene
            });
        }
    }
    return out;
}

void WorldgenScatterSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_layers", "layers"), &WorldgenScatterSettings::set_layers);
    ClassDB::bind_method(D_METHOD("get_layers"), &WorldgenScatterSettings::get_layers_bind);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layers", PROPERTY_HINT_ARRAY_TYPE, "WorldgenScatterLayer"), "set_layers", "get_layers");
}

void WorldgenScatterScene::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_scene", "scene"), &WorldgenScatterScene::set_scene);
    ClassDB::bind_method(D_METHOD("get_scene"), &WorldgenScatterScene::get_scene);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_scene", "get_scene");

    ClassDB::bind_method(D_METHOD("set_weight", "weight"), &WorldgenScatterScene::set_weight);
    ClassDB::bind_method(D_METHOD("get_weight"), &WorldgenScatterScene::get_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight", PROPERTY_HINT_RANGE, "0,1"), "set_weight", "get_weight");
}
