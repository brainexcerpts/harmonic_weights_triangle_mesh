#include "solvers.hpp"

#include <iostream>
#include <Eigen/Core>
#include <Eigen/Sparse>

#include <Eigen/QR>
#include <Eigen/LU>
#include <Eigen/SparseLU>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// -----------------------------------------------------------------------------

typedef Eigen::Triplet<double, int> Triplet;
/// declares a column-major sparse matrix type of double
typedef Eigen::SparseMatrix<double> Sparse_mat;

// -----------------------------------------------------------------------------

/// Are angles obtuse between e0 and e1 (meaning a < PI/2)
static bool check_obtuse(const Vec3& e0,
                         const Vec3& e1)
{
    return e0.normalized().dot( e1.normalized() ) >= 0.f;
}

// -----------------------------------------------------------------------------

/**
 * @brief Check if a triangle is obtuse (every angles < PI/2)
 * Here how edges must be defined :
   @code
      p0
      |\
      | \
      |  \
      |   \
      |____\
     p1     p2

    Vec3 e0 = p1 - p0;
    Vec3 e1 = p2 - p0;
    Vec3 e2 = p2 - p1;
   @endcode
 */
static bool check_obtuse(const Vec3& e0,
                         const Vec3& e1,
                         const Vec3& e2)
{
    return check_obtuse(e0, e1) && check_obtuse(-e0, e2) && check_obtuse(-e1, -e2);
}

// -----------------------------------------------------------------------------

inline static
double mixed_voronoi_area(const Vec3& pi,
                          const Vec3& pj0,
                          const Vec3& pj1)
{
    double area = 0.;
    Vec3 e0 = pj0 - pi ;
    Vec3 e1 = pj1 - pi;
    Vec3 e2 = pj1 - pj0;

    if( check_obtuse(e0, e1, e2) )
    {
        area = (1/8.) * (double)(e0.norm_squared() * (-e0).cotan( e2) +
                                 e1.norm_squared() * (-e1).cotan(-e2));
    }
    else
    {
        const double ta = (double)e0.cross( e1 ).norm() * 0.5; // Tri area
        area = ta / (check_obtuse(e0, e1) ? 2. : 4.);
    }
    return area;
}

// -----------------------------------------------------------------------------

static
double get_cell_area(int vidx,
                     const std::vector< Vec3 >& vertices,
                     const std::vector< std::vector<int> >& edges )
{
    double area = 0.0;
    const Vec3 c_pos = vertices[vidx];
    //get triangles areas
    for(int e = 0; e < (int)edges[vidx].size(); ++e)
    {
        int ne = (e + 1) % edges[vidx].size();

        if(true){
            // Supposidely more precise (but a bit more complex to compute)
            Vec3 p0 = vertices[edges[vidx][e ] ];
            Vec3 p1 = vertices[edges[vidx][ne]];
            area += mixed_voronoi_area(c_pos, p0, p1 );
        }else{
            // This should give descent results as well:
            Vec3 edge0 = vertices[edges[vidx][e ]] - c_pos;
            Vec3 edge1 = vertices[edges[vidx][ne]] - c_pos;
            area += (edge0.cross(edge1)).norm() * 0.5;
            area /= 3.0f;
        }
    }
    return area;
}


// -----------------------------------------------------------------------------

static
float angle_between(const Vec3& v1, const Vec3& v2)
{
    float cosa = v1.dot(v2);
    if(cosa >= 1.f)
        return 0.f;
    else if(cosa <= -1.f)
        return M_PI;
    else
        return std::acos(cosa);
}

// -----------------------------------------------------------------------------

/// @return A sparse representation of the Laplacian matrix 'L'
/// list[ith_row][list of columns] = Triplet(ith_row, jth_column, matrix value)
static
std::vector<std::vector<Triplet>>
get_laplacian(const std::vector< Vec3 >& vertices,
              const std::vector< std::vector<int> >& edges )
{
    std::cout << "BUILD LAPLACIAN MATRIX" << std::endl;
    unsigned nv = unsigned(vertices.size());
    std::vector<std::vector<Triplet>> mat_elemts(nv);
    for(int i = 0; i < nv; ++i)
        mat_elemts[i].reserve(10);

    for(int i = 0; i < nv; ++i)
    {
        const Vec3 c_pos = vertices[i];

        //get laplacian
        double sum = 0.;
        int nb_edges = edges[i].size();
        for(int e = 0; e < nb_edges; ++e)
        {
            int next_edge = (e + 1           ) % nb_edges;
            int prev_edge = (e + nb_edges - 1) % nb_edges;


            /*                                 next_edge
                                    e ◀---v4---(cotan2)
                                   ◥ ◤         /
                                  /   \       /
                                 v2    v5    v3
                                /       \   /
                               /         \ ◣
                        (cotan1)----v1---▶c_pos
                       prev_edge
            */
            Vec3 v1 = c_pos - vertices[edges[i][prev_edge]];
            Vec3 v3 = c_pos - vertices[edges[i][next_edge]];
            double w = 0.0;
            if(true)
            {
                /* Cotangent weights
                 * (may be negative and undesirable in certain situations)
                */
                Vec3 v2 = vertices[edges[i][e]] - vertices[edges[i][prev_edge]];
                Vec3 v4 = vertices[edges[i][e]] - vertices[edges[i][next_edge]];

                double cotan1 = (v1.dot(v2)) / (1e-6 + (v1.cross(v2)).norm() );
                double cotan2 = (v3.dot(v4)) / (1e-6 + (v3.cross(v4)).norm() );

                // TODO: check for edge cases such as
                // the mesh corners and boundaries and adjust cotan weights
                // appropriatly ...
                w = (cotan1 + cotan2) * 0.5f;
            } else {
                // Mean value coordinations weights:
                // doesn't really work something must be wrong
                Vec3 v5 = c_pos - vertices[edges[i][e]];
                v1.normalize();
                v3.normalize();
                float v5_norm = v5.normalize();
                double tan1 = std::tan(angle_between(-v1, v5)*0.5f);
                double tan2 = std::tan(angle_between(-v3, v5)*0.5f);
                w = (tan1 + tan2) / (1e-6 + v5_norm);
            }


            // Disable / Enable multiplying against the inverse of
            // the Mass matrix 'M':
            if(false)
            {
                // If we want to return M^{-1}.L instead of just L
                // Then we can do it here since its more efficient
                // than building M^{-1} and then do the product M^{-1}.L
                // Since we solve for harmonic weights
                // M^{-1}.L = 0 can be simplified to L = 0
                // and this step safely ignored
                double area = get_cell_area(i, vertices, edges);
                area = 1. / ((1e-10 + area));
                w *= area;
            }

            sum += w;

            mat_elemts[i].push_back( Triplet(i, edges[i][e], w) );
        }

        mat_elemts[i].push_back( Triplet(i, i, -sum) );
    }
    return mat_elemts;
}

//------------------------------------------------------------------------------

/*
    Alternate implementation of the Laplacian matrix using only the
    list of triangles instead of the first ring neighboors.
*/
static
std::vector<std::vector<Triplet>>
get_laplacian(const std::vector< Vec3 >& vertices,
              const std::vector< Tri_face >& triangles )
{

    unsigned nv = unsigned(vertices.size());
    std::vector<std::vector<Triplet>> mat_elemts(nv);
    for(int i = 0; i < nv; ++i)
        mat_elemts[i].reserve(10);

    for( const Tri_face& f : triangles)
    {
        struct Edge { int i, j, org; };
        std::vector<Edge> edges =
        {
            {f.a, f.b, f.c},
            {f.b, f.c, f.a},
            {f.c, f.a, f.b},
        };

        for(Edge edge : edges)
        {
            /*
                                    j
                                   ◥
                                  /  \
                                 v2   \
                                /      \
                               /        \
                        (cotan)----v1---▶ i
                           org
            */
            Vec3 v1 = vertices[edge.org] - vertices[edge.i];
            Vec3 v2 = vertices[edge.org] - vertices[edge.j];
            double cotan = (v1.dot(v2)) / (1e-6 + (v1.cross(v2)).norm() );
            float w = cotan * 0.5f;

            int i = edge.i;
            int j = edge.j;
            // Note Eigen::setFromTriplets will sum up duplicate elements for us
            mat_elemts[i].push_back( Triplet(i, j,  w) );
            mat_elemts[j].push_back( Triplet(j, i,  w) );
            mat_elemts[i].push_back( Triplet(i, i, -w) );
            mat_elemts[j].push_back( Triplet(j, j, -w) );
        }
    }
    return mat_elemts;
}

//------------------------------------------------------------------------------

// Compute harmonic weights
void solve_laplace_equation(const std::vector< Vec3 >& vertices,
        const std::vector< std::vector<int> >& edges,
        const std::vector<Tri_face>& triangles,
        const std::vector<std::pair<Vert_idx, float> >& boundaries,
        std::vector<double>& harmonic_weight_map)
{
    std::cout << "COMPUTE LAPLACE EQUATION" << std::endl;

    int nv = vertices.size();

    // compute laplacian matrix of the mesh
    /*
        We can build the laplacian 'L' either from the half edge data structure
        (edges) or simply the list of triangles.
        For reference both versions are implemented here.
    */
    assert(edges.size() > 0 || triangles.size() > 0 );
    std::vector<std::vector<Triplet>> mat_elemts;
    if( edges.size() > 0)
        mat_elemts = get_laplacian(vertices, edges);
    else if( triangles.size() > 0 )
        mat_elemts = get_laplacian(vertices, triangles);


    // Set boundary conditions
    Eigen::VectorXd rhs = Eigen::VectorXd::Constant(nv, 0.);
    // Initialize handle
    for(const std::pair<int, float>& elt : boundaries){
        rhs( elt.first ) = double(elt.second);
        // Set row to 0.0f
        mat_elemts[elt.first].clear();
        // Set
        mat_elemts[elt.first].push_back( Triplet(elt.first, elt.first, 1.0) );
    }


#if 0
    // Solving with a dense matrix is Extremely slow
    Eigen::MatrixXd L = Eigen::MatrixXd::Constant(nv, nv, 0.);
    for( const std::vector<Triplet>& row : mat_elemts)
        for( const Triplet& elt : row )
            L(elt.row(), elt.col()) = elt.value();

    //Eigen::ColPivHouseholderQR<Eigen::MatrixXd> llt;
    Eigen::FullPivLU<Eigen::MatrixXd> solver;
    std::cout << "BEGIN MATRIX FACTORIZATION" << std::endl;
    solver.compute( L );
    std::cout << "END MATRIX FACTORIZATION" << std::endl;

#else
    Sparse_mat L(nv, nv);
    // Convert to triplets
    std::vector<Triplet> triplets;
    triplets.reserve(nv * 10);
    for( const std::vector<Triplet>& row : mat_elemts)
        for( const Triplet& elt : row )
            triplets.push_back( elt );

    L.setFromTriplets(triplets.begin(), triplets.end());

    Eigen::SparseLU<Sparse_mat> solver;
    std::cout << "BEGIN SPARSE MATRIX FACTORIZATION" << std::endl;
    solver.compute( L );
    std::cout << "END SPARSE MATRIX FACTORIZATION" << std::endl;
#endif

    harmonic_weight_map.resize(nv);
    Eigen::VectorXd res = solver.solve( rhs );
    for(int i = 0; i < nv; ++i)
        harmonic_weight_map[i] = res(i);

    return;
}

// -----------------------------------------------------------------------------

