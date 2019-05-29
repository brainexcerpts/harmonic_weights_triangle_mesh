#ifndef MESH_HPP
#define MESH_HPP

#include <fstream>
#include <iostream>
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

// -----------------------------------------------------------------------------

// Recompute 'mesh._normals'
static void compute_normals(Mesh& mesh)
{
    unsigned nb_vertices = 0;
    mesh._normals.assign( nb_vertices, Vec3(0.0f));
    for(unsigned i = 0; i < mesh._triangles.size(); i++ ) {
        const Tri_face& tri = mesh._triangles[i];
        // Compute normals:
        Vec3 v1 = mesh._vertices[tri.b] - mesh._vertices[tri.a];
        Vec3 v2 = mesh._vertices[tri.c] - mesh._vertices[tri.a];

        Vec3 n = v1.cross( v2 );
        n.normalize();

        mesh._normals[ tri.a ] += n;
        mesh._normals[ tri.b ] += n;
        mesh._normals[ tri.c ] += n;
    }
}

// -----------------------------------------------------------------------------

/// @brief Load mesh from an OFF file
static Mesh* build_mesh(const char* file_name)
{
    Mesh* ptr = new Mesh();
    Mesh& mesh = *ptr;

    std::ifstream file( file_name );

    if( !file.is_open() ){
        std::cerr << "Can't open file: " << file_name << std::endl;
        return nullptr;
    }

    std::string format;
    unsigned nb_vertices = 0;
    unsigned nb_faces = 0;
    int nil;

    file >> format;
    file >> nb_vertices >> nb_faces >> nil;

    mesh._vertices.resize( nb_vertices );
    for (unsigned i=0; i < nb_vertices; i++ ){
        Vec3 p;
        file >> p.x >> p.y >> p.z;
        mesh._vertices[i] = p;
    }

    mesh._triangles.resize( nb_faces );
    mesh._colors.assign( nb_vertices, Vec3(0.0f));

    mesh._normals.assign( nb_vertices, Vec3(0.0f));
    for(unsigned i = 0; i < nb_faces; i++ )
    {
        int nb_verts_face;
        file >> nb_verts_face;

        if(nb_verts_face != 3) {
            std::cerr << "We only handle triangle faces" << std::endl;
            continue;
        }

        Tri_face& tri = mesh._triangles[i];
        file >> tri.a >> tri.b >> tri.c;
    }
    compute_normals(mesh);
    file.close();
    return ptr;
}

#endif // MESH_HPP
