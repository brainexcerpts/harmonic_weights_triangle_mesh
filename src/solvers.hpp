#ifndef SOLVERS_HPP
#define SOLVERS_HPP

#include <vector>
#include "mesh.hpp"
#include "vec3.hpp"

/// @brief Compute harmonic weight map of a triangle mesh
/// @param vertices : list of vertex positions
/// @param edges : list of first ring neighbors for each vertex
/// edges[vert_i] = list of adjacent vertices to 'vert_i'
/// @param triangles : optional parameter, used if 'edges' is empty.
/// List of the mesh triangles.
/// @param boundaries : list of vertices a fixed values
/// boundaries[] = (vertex_i, weight)
/// the boundary should describe a closed region of vertices over the mesh
/// 'vertices'
/// @param[out] harmonic_weight_map : values computed inside the boundary
/// these values should represent an harmonic function
void solve_laplace_equation(const std::vector< Vec3 >& vertices,
        const std::vector< std::vector<int> >& edges,
        const std::vector<Tri_face>& triangles,
        const std::vector<std::pair<Vert_idx, float> >& boundaries,
        std::vector<double>& harmonic_weight_map);

#endif // SOLVERS_HPP
