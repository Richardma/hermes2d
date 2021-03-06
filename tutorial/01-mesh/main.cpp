#include "hermes2d.h"

// This example shows how to load a mesh, perform various types
// of "manual"  element refinements, and use keyboard and mouse
// controls.

int main(int argc, char* argv[])
{
  // load the mesh file
  Mesh mesh;
  mesh.load("domain.mesh");

  // perform some sample initial refinements
  mesh.refine_all_elements();          // Refines all elements.
  mesh.refine_towards_vertex(3, 4);    // Refines mesh towards vertex #3 (4x).
  mesh.refine_towards_boundary(2, 4);  // Refines all elements along boundary 2 (4x).
  mesh.refine_element(86, 0);          // Refines element #86 isotropically.
  mesh.refine_element(112, 0);         // Refines element #112 isotropically.
  mesh.refine_element(84, 2);          // Refines element #84 anisotropically.
  mesh.refine_element(114, 1);         // Refines element #114 anisotropically.

  // display the mesh
  MeshView mview("Hello world!", 100, 100, 500, 500);  // (100, 100) is the upper left corner position
  mview.show(&mesh);                                   // 500 x 500 is the window size

  // practice some keyboard and mouse controls
  printf("Click into the image window and:\n");
  printf("  press 'm' to show element numbers,\n");
  printf("  press 'b' to toggle boundary markers,\n");
  printf("  enlarge your window and press 'c' to center the mesh,\n");
  printf("  zoom into the mesh using the right mouse button\n");
  printf("  and move the mesh around using the left mouse button\n");
  printf("    -- in this way you can read the numbers of all elements,\n");
  printf("  press 'c' to center the mesh again,\n");
  printf("  press 'm' to hide element numbers,\n");
  printf("  press 's' to save a screenshot in bmp format\n");
  printf("    -- the bmp file can be converted to any standard\n");
  printf("    image format using the command \"convert\",\n");
  printf("  press 'q' to quit.\n");

  // wait for keyboard or mouse input
  printf("Waiting for keyboard or mouse input.\n");
  View::wait();
  return 0;
}
