#ifndef SCATTER_LAYER_H
#define SCATTER_LAYER_H

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "layer_manager.h"

class ScatterConfigElement : public RefCounted {
    GDCLASS(ScatterConfigElement, RefCounted);
    int min_lod = 0;
    Ref<PackedScene> scene;
    float weight = 1.0f;
protected:
    static void _bind_methods();
public:
    int get_min_lod() const;
    void set_min_lod(int p_min_lod);

    Ref<PackedScene> get_scene() const;
    void set_scene(Ref<PackedScene> p_scene);

    float get_weight() const;
    void set_weight(float p_weight);
    
    virtual String to_string() override;
};

class ScatterConfigLayer : public RefCounted {
    GDCLASS(ScatterConfigLayer, RefCounted);
    int quantity = 0;
    Vector<Ref<ScatterConfigElement>> elements;
protected:
    static void _bind_methods();
public:
    int get_quantity() const;
    void set_quantity(int p_quantity);

    TypedArray<ScatterConfigElement> get_elements_bind() const;
    void set_elements_bind(const TypedArray<ScatterConfigElement> p_elements);
};

class ScatterConfig : public RefCounted {

};

class WorldgenScatterLayer : public ChunkerLayer {
public:
    virtual float get_chunk_size() const override {
        return 512.0f;
    }
};

#endif // SCATTER_LAYER_H
