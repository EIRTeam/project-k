#ifndef PLANE_GENERATE_H
#define PLANE_GENERATE_H

// Taken from primitive_meshes.cpp
#include "core/math/vector2.h"
#include "core/variant/array.h"
#include "scene/resources/3d/primitive_meshes.h"
#include <array>

class PlaneGenerate {
public:
    struct PlaneGenerateSettings {
        Size2 size;
        int subdiv_width = 1;
        int subdiv_depth = 1;
        PlaneMesh::Orientation orientation = PlaneMesh::FACE_Y;
        Vector3 center_offset;
    };
    static void _create_mesh_array(Array &p_arr, const PlaneGenerateSettings &p_settings) {
        int i, j, prevrow, thisrow, point;
        float x, z;

        // Plane mesh can use default UV2 calculation as implemented in Primitive Mesh

        Size2 start_pos = p_settings.size * -0.5;

        Vector3 normal = Vector3(0.0, 1.0, 0.0);
        if (p_settings.orientation == PlaneMesh::FACE_X) {
            normal = Vector3(1.0, 0.0, 0.0);
        } else if (p_settings.orientation == PlaneMesh::FACE_Z) {
            normal = Vector3(0.0, 0.0, 1.0);
        }

        Vector<Vector3> points;
        Vector<Vector3> normals;
        Vector<float> tangents;
        Vector<Vector2> uvs;
        Vector<int> indices;
        point = 0;

    #define ADD_TANGENT(m_x, m_y, m_z, m_d) \
        tangents.push_back(m_x);            \
        tangents.push_back(m_y);            \
        tangents.push_back(m_z);            \
        tangents.push_back(m_d);

        /* top + bottom */
        z = start_pos.y;
        thisrow = point;
        prevrow = 0;
        for (j = 0; j <= (p_settings.subdiv_depth + 1); j++) {
            x = start_pos.x;
            for (i = 0; i <= (p_settings.subdiv_width + 1); i++) {
                float u = i;
                float v = j;
                u /= (p_settings.subdiv_width + 1.0);
                v /= (p_settings.subdiv_depth + 1.0);

                if (p_settings.orientation == PlaneMesh::FACE_X) {
                    points.push_back(Vector3(0.0, z, x) + p_settings.center_offset);
                } else if (p_settings.orientation == PlaneMesh::FACE_Y) {
                    points.push_back(Vector3(-x, 0.0, -z) + p_settings.center_offset);
                } else if (p_settings.orientation == PlaneMesh::FACE_Z) {
                    points.push_back(Vector3(-x, z, 0.0) + p_settings.center_offset);
                }
                normals.push_back(normal);
                if (p_settings.orientation == PlaneMesh::FACE_X) {
                    ADD_TANGENT(0.0, 0.0, -1.0, 1.0);
                } else {
                    ADD_TANGENT(1.0, 0.0, 0.0, 1.0);
                }
                uvs.push_back(Vector2(1.0 - u, 1.0 - v)); /* 1.0 - uv to match orientation with Quad */
                point++;

                if (i > 0 && j > 0) {
                    indices.push_back(prevrow + i - 1);
                    indices.push_back(prevrow + i);
                    indices.push_back(thisrow + i - 1);
                    indices.push_back(prevrow + i);
                    indices.push_back(thisrow + i);
                    indices.push_back(thisrow + i - 1);
                }

                x += p_settings.size.x / (p_settings.subdiv_width + 1.0);
            }

            z += p_settings.size.y / (p_settings.subdiv_depth + 1.0);
            prevrow = thisrow;
            thisrow = point;
        }

        p_arr[RS::ARRAY_VERTEX] = points;
        p_arr[RS::ARRAY_NORMAL] = normals;
        p_arr[RS::ARRAY_TANGENT] = tangents;
        p_arr[RS::ARRAY_TEX_UV] = uvs;
        p_arr[RS::ARRAY_INDEX] = indices;
    }

    enum GridTJunctionRemovalFlags {
        UP = 1,
        DOWN = 1 << 1,
        LEFT = 1 << 2,
        RIGHT = 1 << 3
    };

    enum GridDirection {
        SW,
        W,
        NW,
        N,
        NE,
        E,
        SE,
        S,
        C,
        GRID_DIR_MAX
    };

    struct GridMesh {
        Vector<Vector3> positions;
        Vector<int32_t> indices;
        Vector<Vector2> uvs;
        Vector<Vector3> normals;
    };

    typedef std::array<int32_t, GridDirection::GRID_DIR_MAX> GridElementIndices;


    struct GridMeshSettings {
        int element_count = 1;
        float side_length = 1.0f;
    };

    static void create_triangle_no_tjunction(int p_idx, Vector<int32_t> &p_indices, const GridElementIndices &p_element_indices) {
        p_indices.push_back(p_element_indices[p_idx]);
	    p_indices.push_back(p_element_indices[(p_idx + 2) % 8]);
	    p_indices.push_back(p_element_indices[GridDirection::C]);
    }

    static void create_triangle(int p_idx, Vector<int32_t> &p_indices, const GridElementIndices &p_element_indices) {
        p_indices.push_back(p_element_indices[p_idx]);
	    p_indices.push_back(p_element_indices[(p_idx + 1) % 8]);
	    p_indices.push_back(p_element_indices[GridDirection::C]);
    }

    static void generate_mesh(const GridMeshSettings &p_settings, BitField<GridTJunctionRemovalFlags> p_removal_flags, GridMesh &r_mesh) {
        //ERR_FAIL_COND(p_settings.element_count % 2 != 0);

        float per_element_size = p_settings.side_length / (float)p_settings.element_count;

        bool remove_right_tjunction = p_removal_flags.has_flag(GridTJunctionRemovalFlags::RIGHT);
        bool remove_left_tjunction = p_removal_flags.has_flag(GridTJunctionRemovalFlags::LEFT);
        bool remove_down_tjunction = p_removal_flags.has_flag(GridTJunctionRemovalFlags::DOWN);
        bool remove_up_tjunction = p_removal_flags.has_flag(GridTJunctionRemovalFlags::UP);

        LocalVector<GridElementIndices> prev_row_indices;
        for (int element_y = 0; element_y < p_settings.element_count; element_y++) {
            LocalVector<GridElementIndices> row_indices;
            GridElementIndices prev_element_indices;
            for (int element_x = 0; element_x < p_settings.element_count; element_x++) {
                GridElementIndices element_indices;
                float element_x_start = element_x * per_element_size;
                float element_y_start = element_y * per_element_size;


                // Left vertices, first column
                if (element_x == 0) {
                    // Bottom left and medium left
                    r_mesh.positions.push_back(Vector3(element_x_start, 0.0f, element_y_start+per_element_size));
                    r_mesh.positions.push_back(Vector3(element_x_start, 0.0f, element_y_start+per_element_size*0.5f));

                    element_indices[GridDirection::SW] = r_mesh.positions.size()-2;
                    element_indices[GridDirection::W] = r_mesh.positions.size()-1;

                    // Top left, shared with previous row
                    if (element_y == 0) {
                        r_mesh.positions.push_back(Vector3(element_x_start, 0.0f, element_y_start));
                        element_indices[GridDirection::NW] = r_mesh.positions.size()-1;
                    } else {
                        // Shared with previous row
                        element_indices[GridDirection::NW] = prev_row_indices[element_x][GridDirection::SW];
                    }
                } else {
                    // Left vertices, shared with previous column.
                    element_indices[GridDirection::SW] = prev_element_indices[GridDirection::SE];
				    element_indices[GridDirection::W] = prev_element_indices[GridDirection::E];
				    element_indices[GridDirection::NW] = prev_element_indices[GridDirection::NE];
                }

                // Top vertices, first row
                if (element_y == 0) {
                    r_mesh.positions.push_back(Vector3(element_x_start + per_element_size*0.5f, 0.0f, element_y_start));
                    r_mesh.positions.push_back(Vector3(element_x_start + per_element_size, 0.0f, element_y_start));
                    element_indices[GridDirection::N] = r_mesh.positions.size()-2;
                    element_indices[GridDirection::NE] = r_mesh.positions.size()-1;
                } else {
                    // Top elements, shared with previous row
                    element_indices[GridDirection::N] = prev_row_indices[element_x][GridDirection::S];
				    element_indices[GridDirection::NE] = prev_row_indices[element_x][GridDirection::SE];
                }

                // Right vertices
                r_mesh.positions.push_back(Vector3(element_x_start + per_element_size, 0.0f, element_y_start + per_element_size*0.5f));
                r_mesh.positions.push_back(Vector3(element_x_start + per_element_size, 0.0f, element_y_start + per_element_size));
                element_indices[GridDirection::E] = r_mesh.positions.size()-2;
                element_indices[GridDirection::SE] = r_mesh.positions.size()-1;

                // South vertex
                r_mesh.positions.push_back(Vector3(element_x_start + per_element_size*0.5f, 0.0f, element_y_start + per_element_size));
                element_indices[GridDirection::S] = r_mesh.positions.size()-1;

                // Center vertex
                r_mesh.positions.push_back(Vector3(element_x_start + per_element_size*0.5f, 0.0f, element_y_start + per_element_size * 0.5f));
                element_indices[GridDirection::C] = r_mesh.positions.size()-1;

                for (int i = 0; i < 8; i++) {
                    if (i == GridDirection::NE && remove_right_tjunction && element_x == p_settings.element_count-1) {
                        create_triangle_no_tjunction(i, r_mesh.indices, element_indices);
                        i++;
                        continue;
                    }
                    if (i == GridDirection::SW && remove_left_tjunction && element_x == 0) {
                        create_triangle_no_tjunction(i, r_mesh.indices, element_indices);
                        i++;
                        continue;
                    }
                    if (i == GridDirection::NW && remove_up_tjunction && element_y == 0) {
                        create_triangle_no_tjunction(i, r_mesh.indices, element_indices);
                        i++;
                        continue;
                    }
                    if (i == GridDirection::SE && remove_down_tjunction && element_y == p_settings.element_count-1) {
                        create_triangle_no_tjunction(i, r_mesh.indices, element_indices);
                        i++;
                        continue;
                    }

                    create_triangle(i, r_mesh.indices, element_indices);
                }
                prev_element_indices = element_indices;
                row_indices.push_back(element_indices);
            }

            prev_row_indices = row_indices;
        }

        r_mesh.uvs.resize(r_mesh.positions.size());
        Vector2 *uvs_ptrw = r_mesh.uvs.ptrw();
        for(int i = 0; i < r_mesh.positions.size(); i++) {
            uvs_ptrw[i] = Vector2(r_mesh.positions[i].x, r_mesh.positions[i].z) / Vector2(p_settings.side_length, p_settings.side_length);
        }
    }

};
#endif // PLANE_GENERATE_H
