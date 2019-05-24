#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include <cassert>

#include "vec3.hpp"

// -----------------------------------------------------------------------------

typedef int Vert_idx; ///< Vertex index
typedef int Tri_idx;  ///< Triangle index

// -----------------------------------------------------------------------------

/// @brief triangle face represented with 3 vertex indices
/// you can access the indices with the attributes '.a' '.b' '.c' or the array
/// accessor []
struct Tri_face {

    Tri_face() : a(-1), b(-1), c(-1) { }

    /// Acces through array operator []
    const Vert_idx& operator[](int i) const {
        assert( i < 3);
        return ((Vert_idx*)this)[i];
    }

    Vert_idx& operator[](int i) {
        assert( i < 3);
        return ((Vert_idx*)this)[i];
    }

    Vert_idx a, b, c;
};

// -----------------------------------------------------------------------------

struct Mesh {
    std::vector<Vec3> _vertices;
    std::vector<Vec3> _normals;
    std::vector<Vec3> _colors;
    std::vector<Tri_face> _triangles;
    unsigned nb_vertices() const { return  _vertices.size(); }
    unsigned nb_triangles() const { return  _triangles.size(); }
};

#endif // MESH_HPP
