#ifndef VERTEX_TO_FACE_HPP
#define VERTEX_TO_FACE_HPP

#include <vector>
#include "mesh.hpp"

/// @brief Compute associative arrays from vertex to face.
struct Vertex_to_face {
    /// list of triangle indices connected to a given vertex.
    /// _1st_ring_tris[index_vert][nb_connected_triangles] = index triangle
    /// @warning triangle list per vert are unordered
    std::vector<std::vector<Tri_idx> > _1st_ring_tris;

    /// Does the ith vertex belongs to a face? (triangle or a quad)
    std::vector<bool> _is_vertex_connected;

    void clear() {
        _1st_ring_tris.clear();
        _is_vertex_connected.clear();
    }

    /// Allocate and compute attributes
    void compute(const Mesh& mesh);
};

#endif // VERTEX_TO_FACE_HPP
