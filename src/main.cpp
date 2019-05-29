#include <GL/glut.h>

#include <iostream>
#include <vector>
#include <cmath>

#include "mesh.hpp"
#include "topology/vertex_to_face.hpp"
#include "topology/vertex_to_1st_ring_vertices.hpp"
#include "solvers.hpp"

// compatibility with original GLUT
#if !defined(GLUT_WHEEL_UP)
#  define GLUT_WHEEL_UP   3
#  define GLUT_WHEEL_DOWN 4
#endif

// =============================================================================
// hard coded settings
// =============================================================================

/*
 * For convenience sample models lie onto the (x,y) plane and
 * their scale roughly range from [-1.0 1.0]
*/
#if 0
const char* _sample_path = "samples/buddha.off";
const char* _sample_path = "samples/donut.off";
const char* _sample_path = "samples/plane_regular.off";
const char* _sample_path = "samples/plane_regular_res1.off";

// Sample mesh with wholes will not give perfect results, more advanced methods
// will be needed to handle such messy input
// For instance:
// "Natural Boundary Conditions for Smoothing in Geometry Processing"
// Could be a good start.
const char* _sample_path = "samples/plane_wholes.off";
#endif
const char* _sample_path = "samples/plane_iregular_1.off";


enum Boundary_type {
    eSTRIP,
    eCONE
};


Boundary_type _b_type = eCONE; /*eSTRIP*/

// When 'true' Automatically turn around the 3D model (you can adjust angle of
// view with the scroll wheel.
// In addition, displace the sample model vertices along the Z axis giventhe
// harmonic weight map value.

// When 'false' we simply look at the model from the top and display vertex color
// as a heat map (0 -> blue, 1 -> red )
bool _3d_view = true;

// When false we don't really on the first ring neighborhood to build the
// Laplacian matrix but only the list of triangles.
bool _use_half_edges = true;

// =============================================================================
// Global variables
// =============================================================================

// In 3D view the current angle of view from the "turntable"
float _table_angle = 0.0f;
static int _win_number;
Mesh* _mesh;

// List of GL_POINTS to display
// (represents boundary conditions of the Laplace PDE, i.e the vertices
// where we fix values by hand)
std::vector<std::pair<Vec3/*position*/, Vec3/*color*/>> g_ogl_points;

// =============================================================================
// GLUT
// =============================================================================

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
    static bool _wires  = false;

    switch (c) {
    case 27 :
        glFinish();
        glutDestroyWindow(_win_number);
        exit (0);
        break;
    case 'w' :
        if(!_wires) {
            glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_LIGHTING);
            glutPostRedisplay();
            _wires = true;
        }else {
            glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_LIGHTING);
            glutPostRedisplay();
            _wires = false;
        }
        break;
    }
}

// -----------------------------------------------------------------------------

void mouse_keys (int button, int state, int x, int y)
{
    if(button == GLUT_WHEEL_UP)
        _table_angle += 1.0f;
    if(button == GLUT_WHEEL_DOWN)
        _table_angle -= 1.0f;
}

// -----------------------------------------------------------------------------

void display(void)
{
    glClearColor ( 0.93,  0.93,  0.93,  0.93);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glViewport(0,0,900,900);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60., 1., 0.5, 100.);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    if(_3d_view){
        GLfloat lightPos0[] = {0.0f, 0.0f, 0.0f, 1.0f};
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

        glTranslatef(0.0, 0.0, -1.5);
        glRotatef(45.0f, -1.0f, 0.0f, 0.0f);
        glRotatef(_table_angle, -1.0f, 0.0f, 0.0f);

        static float angle = 0.0f;
        angle = fmodf(angle+0.3f, 360.f);
        //glRotatef(angle, 1.0f, 0.0f, 0.0f);
        glRotatef(angle, 0.0f, 0.0f, 1.0f);
        glutPostRedisplay();
    }
    else
        glTranslatef(0.0, 0.0, -1.0);
    float s = 0.5f;
    glScalef(s, s, s);
    draw_mesh(*_mesh);


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

// =============================================================================
// =============================================================================

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

void set_strip_boundaries(std::vector<std::pair<Vert_idx, float> >& boundaries,
                          const Mesh& mesh,
                          float length = 0.1f)
{
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

    boundaries.resize(mesh.nb_vertices());
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
}

// -----------------------------------------------------------------------------

/*

    Set boundary conditions to look like this:


        0   0   0   0   0   0   0   0

        0                           0
                    1  1
        0        1       1          0
                1          1
        0       1          1        0
                 1        1
        0           1  1            0

        0                           0
                                      ▲
        0   0   0   0   0   0   0   0 | length
                                 <--> ▼
                                length

    0s on all sides and 1s in the middle

*/
void set_cone_boundaries(std::vector<std::pair<Vert_idx, float> >& boundaries,
                         const Mesh& mesh,
                         float length = 0.01f)
{
    boundaries.resize(mesh.nb_vertices());
    int acc = 0;
    for(int i = 0; i < mesh.nb_vertices(); ++i)
    {
        Vec3 pos = mesh._vertices[i];
        float dist = (pos.y + 1.0f) * 0.5f;
        // Set bottom strip
        if( dist < length) {
            boundaries[acc++] = std::make_pair(i, 0.0f);
            g_ogl_points.emplace_back( mesh._vertices[i], Vec3(0.0f, 1.0f, 0.0f) );
        }

        // Set top strip
        if( dist > (1.0f-length) ) {
            boundaries[acc++] = std::make_pair(i, 0.0f);
            g_ogl_points.emplace_back( mesh._vertices[i], Vec3(0.0f, 1.0f, 0.0f) );
        }

        dist = (pos.x + 1.0f) * 0.5f;
        // set left strip
        if( dist < length) {
            boundaries[acc++] = std::make_pair(i, 0.0f);
            g_ogl_points.emplace_back( mesh._vertices[i], Vec3(0.0f, 1.0f, 0.0f) );
        }

        // set right strip
        if( dist > (1.0f-length) ) {
            boundaries[acc++] = std::make_pair(i, 0.0f);
            g_ogl_points.emplace_back( mesh._vertices[i], Vec3(0.0f, 1.0f, 0.0f) );
        }

        // Set central values
        dist = (pos.x*pos.x + pos.y*pos.y);
        if( dist < length) {
            boundaries[acc++] = std::make_pair(i, 1.0f);
            g_ogl_points.emplace_back( mesh._vertices[i], Vec3(1.0f, 0.0f, 0.0f) );
        }
    }
    boundaries.resize( acc );
}

// -----------------------------------------------------------------------------

void deform_mesh(std::vector<Vec3>& vertices, const std::vector<double>& weight_map){
    float s = 1.0f;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        Vec3& v = vertices[i];
        v.z = v.z + float(weight_map[i]) * s;
    }
}

// -----------------------------------------------------------------------------

void compute_harmonic_map()
{
    _mesh = build_mesh(_sample_path);
    Mesh& mesh = *_mesh;

    // Compute first ring
    Vertex_to_face v_to_face;
    v_to_face.compute( mesh );
    Vertex_to_1st_ring_vertices first_ring;
    first_ring.compute(mesh, v_to_face );

    std::vector< std::vector<int> > edges = first_ring._rings_per_vertex;

    /// Define boundary conditions
    std::vector<std::pair<Vert_idx, float> > boundaries;
    switch (_b_type) {
    case eSTRIP: set_strip_boundaries(boundaries, mesh); break;
    case eCONE:  set_cone_boundaries(boundaries, mesh);  break;
    }

    if( !_use_half_edges ){
        edges.clear();
    }
    std::vector<double> weight_map( mesh.nb_vertices() );
    solve_laplace_equation(mesh._vertices,
                           edges,
                           mesh._triangles,
                           boundaries,
                           weight_map);

    if(_3d_view)
    {
        // Displace vertices along Z axis
        deform_mesh(_mesh->_vertices, weight_map);
        compute_normals(*_mesh);
        for(unsigned v = 0; v < mesh.nb_vertices(); ++v)
            mesh._colors[v] = (mesh._normals[v]+1.0f)*0.5f;
    }
    else
    {
        /// Set mesh color according to the computed weight map
        for(unsigned v = 0; v < mesh.nb_vertices(); ++v)
        {
            float x = weight_map[v];
            mesh._colors[v] = heat_color(x) * level_curves_greyscale(x, 10.0f);
        }
    }
}

// -----------------------------------------------------------------------------

void setup_glut(int argc, char** argv)
{
    glutInit (&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (900, 900);
    glutInitWindowPosition (240, 212);
    _win_number = glutCreateWindow (argv[0]);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    //glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glutKeyboardFunc(key_stroke);
    glutMouseFunc(mouse_keys);
    glutDisplayFunc(display);
}

// -----------------------------------------------------------------------------

int main (int argc, char** argv)
{
    compute_harmonic_map();
    setup_glut(argc, argv);
    glutMainLoop();
    return (0);
}

