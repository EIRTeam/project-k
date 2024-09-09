#include "scatter_layer.h"
#include "core/object/object.h"
#include "scene/resources/packed_scene.h"
#include "core/variant/variant.h"

void ScatterConfigElement::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_min_lod"), &ScatterConfigElement::get_min_lod);
    ClassDB::bind_method(D_METHOD("set_min_lod", "min_lod"), &ScatterConfigElement::set_min_lod);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "min_lod", PROPERTY_HINT_RANGE, "0,10"), "set_min_lod", "get_min_lod");

    ClassDB::bind_method(D_METHOD("get_scene"), &ScatterConfigElement::get_scene);
    ClassDB::bind_method(D_METHOD("set_scene", "scene"), &ScatterConfigElement::set_scene);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_scene", "get_scene");

    ClassDB::bind_method(D_METHOD("get_weight"), &ScatterConfigElement::get_weight);
    ClassDB::bind_method(D_METHOD("set_weight", "weight"), &ScatterConfigElement::set_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");
}

int ScatterConfigElement::get_min_lod() const {
    return min_lod;
}

void ScatterConfigElement::set_min_lod(int p_min_lod) {
    min_lod = p_min_lod;
}

String ScatterConfigElement::to_string() {
    return vformat("ScatterConfigElement: (LOD %d)", min_lod);
}

Ref<PackedScene> ScatterConfigElement::get_scene() const { return scene; }

void ScatterConfigElement::set_scene(Ref<PackedScene> p_scene) { scene = p_scene; }

float ScatterConfigElement::get_weight() const { return weight; }

void ScatterConfigElement::set_weight(float p_weight) { weight = p_weight; }

void ScatterConfigLayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_quantity"), &ScatterConfigLayer::get_quantity);
    ClassDB::bind_method(D_METHOD("set_quantity", "quantity"), &ScatterConfigLayer::set_quantity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "quantity", PROPERTY_HINT_RANGE, "0,10,or_greater"), "set_quantity", "get_quantity");

    ClassDB::bind_method(D_METHOD("get_elements"), &ScatterConfigLayer::get_elements_bind);
    ClassDB::bind_method(D_METHOD("set_elements", "bind"), &ScatterConfigLayer::set_elements_bind);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "elements", PROPERTY_HINT_ARRAY_TYPE, "ScatterConfigElement"), "set_elements_bind", "get_elements_bind");
}

int ScatterConfigLayer::get_quantity() const {
    return quantity;
}

void ScatterConfigLayer::set_quantity(int p_amount) {
    quantity = p_amount;
}

TypedArray<ScatterConfigElement> ScatterConfigLayer::get_elements_bind() const {
    TypedArray<ScatterConfigElement> out;
    for (Ref<ScatterConfigElement> e : elements) {
        out.push_back(e);
    }
    return out;
}

void ScatterConfigLayer::set_elements_bind(const TypedArray<ScatterConfigElement> p_elements) {
    elements.clear();
    for (Ref<ScatterConfigElement> element : p_elements) {
        elements.push_back(element);
    }
}
