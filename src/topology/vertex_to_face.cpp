#include "topology/vertex_to_face.hpp"

#include <cassert>

void Vertex_to_face::compute(const Mesh& mesh)
{
    clear();
    _1st_ring_tris. resize( mesh.nb_vertices() );
    _is_vertex_connected.assign(mesh.nb_vertices(), false );

    for(int i = 0; i < mesh.nb_triangles(); i++)
    {
        Tri_face tri = mesh._triangles[ i ];
        for(int j = 0; j < 3; j++){
            int v = tri[ j ];
            assert(v >= 0);
            _1st_ring_tris[v].push_back(i);
            _is_vertex_connected[v] = true;
        }
    }

    // FIXME: look up the list _tri_list_per_vert and re-order in order to ensure
    // triangles touching each other are next in the list.
}
