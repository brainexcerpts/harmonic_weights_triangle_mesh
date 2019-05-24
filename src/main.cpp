#include <GL/glut.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>

#include "mesh.hpp"
#include "topology/vertex_to_face.hpp"
#include "topology/vertex_to_1st_ring_vertices.hpp"
#include "solvers.hpp"

// =============================================================================

static int g_win_number;
Mesh* g_mesh;

// List of GL_POINTS to display
// (represents boundary conditions of the Laplace PDE, i.e the vertices
// where we fix values by hand)
std::vector<std::pair<Vec3/*position*/, Vec3/*color*/>> g_ogl_points;

// =============================================================================

/// @brief Load mesh from a file
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
    mesh._normals.assign( nb_vertices, Vec3(0.0f));
    mesh._colors.assign( nb_vertices, Vec3(0.0f));

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

        // Compute normals:
        Vec3 v1 = mesh._vertices[tri.b] - mesh._vertices[tri.a];
        Vec3 v2 = mesh._vertices[tri.c] - mesh._vertices[tri.a];

        Vec3 n = v1.cross( v2 );
        n.normalize();

        mesh._normals[ tri.a ] += n;
        mesh._normals[ tri.b ] += n;
        mesh._normals[ tri.c ] += n;
    }
    file.close();
    return ptr;
}

// ----------

void draw_mesh(const Mesh& mesh) {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, mesh._vertices.data());
    glNormalPointer(GL_FLOAT, 0, mesh._normals.data());
    glColorPointer(3, GL_FLOAT, 0, mesh._colors.data());
    glDrawElements(GL_TRIANGLES, mesh._triangles.size()*3, GL_UNSIGNED_INT, mesh._triangles.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

// -----------------------------------------------------------------------------

void key_stroke (unsigned char c, int mouseX, int mouseY) {
    static bool wires  = false;

    switch (c) {
    case 27 :
        glFinish();
        glutDestroyWindow(g_win_number);
        exit (0);
        break;
    case 'w' :
        if(!wires) {
            glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
            glutPostRedisplay();
            wires = true;
        }else {
            glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
            glutPostRedisplay();
            wires = false;
        }
        break;
    }
}

// -----------------------------------------------------------------------------

void display(void)
{
    glClearColor (0.8, 0.8, 0.8, 0.0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glViewport(0,0,900,900);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60., 1., 0.5, 100.);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glTranslatef(0.0, 0.0, -1.0);
    float s = 0.5f;
    glScalef(s, s, s);
    draw_mesh(*g_mesh);

    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for(auto elt : g_ogl_points){
        Vec3 v = elt.first;
        Vec3 c = elt.second;
        glColor3f ( c.x, c.y, c.z);
        glVertex3f( v.x, v.y, v.z+0.00001f /*put it slightly forward*/ );
    }
    glEnd();

    glutSwapBuffers();
    glFlush ();
}

// -----------------------------------------------------------------------------

static Vec3 heat_color(float w)
{
    Vec3 c(0.5f, 0.f, 1.f);
    if(w < 0.25f){
        w *= 4.f;
        c.set(0.f, w, 1.f);
    }else if(w < 0.5f){
        w = (w-0.25f) * 4.f;
        c.set(0.f, 1.f, 1.f-w);
    }else if(w < 0.75f){
        w = (w-0.5f) * 4.f;
        c.set(w, 1.f, 0.f);
    }else{
        w = (w-0.75f) * 4.f;
        c.set(1.f, 1.f-w, 0.f);
    }
    return c;
}

// -----------------------------------------------------------------------------

float level_curves_greyscale(float f, float scale = 1.)
{
    float opacity = cosf(f * 3.141592f * scale);
    opacity *= opacity;
    opacity  = 1.f - opacity;
    opacity *= opacity;
    opacity *= opacity;
    opacity  = 1.f - opacity;
    return opacity;
}

// -----------------------------------------------------------------------------

void compute_harmonic_map()
{
    //g_mesh = build_mesh("samples/buddha.off");
    //g_mesh = build_mesh("samples/donut.off");
    //g_mesh = build_mesh("samples/plane_regular.off");
    //g_mesh = build_mesh("samples/plane_regular_res1.off");
    g_mesh = build_mesh("samples/plane_iregular_1.off");
    //g_mesh = build_mesh("samples/plane_wholes.off");
    Mesh& mesh = *g_mesh;

    // Compute first ring
    Vertex_to_face v_to_face;
    v_to_face.compute( mesh );
    Vertex_to_1st_ring_vertices first_ring;
    first_ring.compute(mesh, v_to_face );


    std::vector< Vec3 > vertices = mesh._vertices;
    std::vector< std::vector<int> > edges = first_ring._rings_per_vertex;


    /// Define boundary conditions
    /*
     *  Our sample models coordinates should roughly lie between [-1, 1]
     *  We are going to select a thin strip of vertices on the top and bottom
     *  of the model:

        -----------------------▲  ▲ (1)
            ___                   |
           //_\\_                 |
         ."\\    ".  ----------▼  |
        /          \              |
        |           \_            |
        |       ,--.-.)           |
        \     /  o \o\            + (0)
        /\/\  \    /_/            |
         (_.   `--'__)            |
          |     .-'  \            |
          |  .-'.     )           |
          | (  _/--.-'            |
          |  `.___.' ----------▲  |
                (                 |
                                  |
        -----------------------▼  ▼ (-1)

     *
    */

    // Size of the strip:
    float length = 0.1f;

    // boundaries[] = (vertex idx, fixed value for the weight map)
    std::vector<std::pair<Vert_idx, float> > boundaries( mesh.nb_vertices() );
    int acc = 0;
    for(int i = 0; i < mesh.nb_vertices(); ++i)
    {
        float dist = (mesh._vertices[i].y + 1.0f) * 0.5f;
        if( dist < length) {
            boundaries[acc++] = std::make_pair(i, dist);
            g_ogl_points.emplace_back( mesh._vertices[i], Vec3(1.0f, 0.0f, 0.0f) );
        }

        if( dist > (1.0f-length) ) {
            boundaries[acc++] = std::make_pair(i, dist);
            g_ogl_points.emplace_back( mesh._vertices[i], Vec3(0.0f, 1.0f, 0.0f) );
        }
    }
    boundaries.resize( acc );

    std::vector<double> weight_map( mesh.nb_vertices() );
    solve_laplace_equation(vertices, edges, boundaries, weight_map);

    /// Set mesh color according to the computed weight map
    /// Also compute some error metrics
    float max_error  = 0.0f;
    float mean_error = 0.0f;
    for(unsigned v = 0; v < mesh.nb_vertices(); ++v)
    {
        float dist = (mesh._vertices[v].y + 1.0f) * 0.5f;
        float x = weight_map[v];

        float error = std::abs(dist-x);
        max_error = std::max( max_error, error);
        mean_error += error;

        if(true)
            mesh._colors[v] = heat_color(x) * level_curves_greyscale(x, 10.0f);
        else{
            //mesh._colors[v] = Vec3( std::sqrt(error*5.0) );
            mesh._colors[v] = heat_color( std::sqrt(error*5.0) );
        }
    }

    std::cout << "Mean error from the cartesian distance: " << mean_error/(float)mesh.nb_vertices() << std::endl;
    std::cout << "Max error from the cartesian distance: " << max_error << std::endl;
}

// -----------------------------------------------------------------------------

int main (int argc, char** argv)
{

#ifndef NDEBUG
   std::cout << "debug" << std::endl;
#else
    std::cout << "release" << std::endl;
#endif
    compute_harmonic_map();

    glutInit (&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (900, 900);
    glutInitWindowPosition (240, 212);
    g_win_number = glutCreateWindow (argv[0]);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glutKeyboardFunc(key_stroke);
    glutDisplayFunc(display);
    glutMainLoop();

    return (0);
}

