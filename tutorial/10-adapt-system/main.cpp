#include "hermes2d.h"
#include "solver_umfpack.h"

// This example explains how to use the multimesh adaptive hp-FEM,
// where different physical fields (or solution components) can be
// approximated using different meshes and equipped with mutually
// independent adaptivity mechanisms. Here we consider linear elasticity
// and will approximate each displacement components using an individual
// mesh.
//
// PDE: Lame equations of linear elasticity, treated as a coupled system
//      of two PDEs
//
// BC: u_1 = u_2 = 0 on Gamma_1
//     du_2/dn = f on Gamma_2
//     du_1/dn = du_2/dn = 0 elsewhere
//
// The following parameters can be changed: In particular, compare hp- and
// h-adaptivity via the ADAPT_TYPE option, and compare the multi-mesh vs. single-mesh
// using the MULTI parameter.

const int P_INIT = 1;            // Initial polynomial degree of all mesh elements.
const bool MULTI = true;         // MULTI = true  ... use multi-mesh,
                                 // MULTI = false ... use single-mesh.
                                 // Note: In the single mesh option, the meshes are
                                 // forced to be geometrically the same but the
                                 // polynomial degrees can still vary.
const bool SAME_ORDERS = true;   // SAME_ORDERS = true ... when single-mesh is used,
                                 // this forces the meshes for all components to be
                                 // identical, including the polynomial degrees of
                                 // corresponding elements. When multi-mesh is used,
                                 // this parameter is ignored.
const double THRESHOLD = 0.3;    // This is a quantitative parameter of the adapt(...) function and
                                 // it has different meanings for various adaptive strategies (see below).
const int STRATEGY = 1;          // Adaptive strategy:
                                 // STRATEGY = 0 ... refine elements until sqrt(THRESHOLD) times total
                                 //   error is processed. If more elements have similar errors, refine
                                 //   all to keep the mesh symmetric.
                                 // STRATEGY = 1 ... refine all elements whose error is larger
                                 //   than THRESHOLD times maximum element error.
                                 // STRATEGY = 2 ... refine all elements whose error is larger
                                 //   than THRESHOLD.
                                 // More adaptive strategies can be created in adapt_ortho_h1.cpp.
const int ADAPT_TYPE = 0;        // Type of automatic adaptivity:
                                 // ADAPT_TYPE = 0 ... adaptive hp-FEM (default),
                                 // ADAPT_TYPE = 1 ... adaptive h-FEM,
                                 // ADAPT_TYPE = 2 ... adaptive p-FEM.
const bool ISO_ONLY = false;     // Isotropic refinement flag (concerns quadrilateral elements only).
                                 // ISO_ONLY = false ... anisotropic refinement of quad elements
                                 // is allowed (default),
                                 // ISO_ONLY = true ... only isotropic refinements of quad elements
                                 // are allowed.
const int MESH_REGULARITY = -1;  // Maximum allowed level of hanging nodes:
                                 // MESH_REGULARITY = -1 ... arbitrary level hangning nodes (default),
                                 // MESH_REGULARITY = 1 ... at most one-level hanging nodes,
                                 // MESH_REGULARITY = 2 ... at most two-level hanging nodes, etc.
                                 // Note that regular meshes are not supported, this is due to
                                 // their notoriously bad performance.
const int MAX_ORDER = 10;        // Maximum allowed element degree
const double ERR_STOP = 0.05;    // Stopping criterion for adaptivity (rel. error tolerance between the
                                 // fine mesh and coarse mesh solution in percent).
const int NDOF_STOP = 40000;     // Adaptivity process stops when the number of degrees of freedom grows over
                                 // this limit. This is mainly to prevent h-adaptivity to go on forever.

// problem constants
const double E  = 200e9;  // Young modulus for steel: 200 GPa
const double nu = 0.3;    // Poisson ratio
const double f  = 1e3;    // load force: 10^3 N
const double lambda = (E * nu) / ((1 + nu) * (1 - 2*nu));
const double mu = E / (2*(1 + nu));

// boundary markers
const int marker_left = 1;
const int marker_top = 2;

// boundary conditions
int bc_types_xy(int marker)
  { return (marker == marker_left) ? BC_ESSENTIAL : BC_NATURAL; }

// bilinear forms
scalar bilinear_form_0_0(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdx_b_dudy_dvdy(lambda+2*mu, fu, mu, fv, ru, rv); }

scalar bilinear_form_0_1(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdy_b_dudy_dvdx(lambda, fv, mu, fu, rv, ru); }

scalar bilinear_form_1_0(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdy_b_dudy_dvdx(lambda, fu, mu, fv, ru, rv); }

scalar bilinear_form_1_1(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdx_b_dudy_dvdy(mu, fu, lambda+2*mu, fv, ru, rv); }

// linear form
scalar linear_form_1_surf_top(RealFunction* fv, RefMap* rv, EdgePos* ep)
  { return -f * surf_int_v(fv, rv, ep); }

int main(int argc, char* argv[])
{
  // load the mesh
  Mesh xmesh, ymesh;
  xmesh.load("bracket.mesh");

  // create initial mesh for the vertical displacement component,
  // identical to the mesh for the horizontal displacement
  // (bracket.mesh becomes a master mesh)
  ymesh.copy(&xmesh);

  // initialize the shapeset and the cache
  H1Shapeset shapeset;
  PrecalcShapeset xpss(&shapeset);
  PrecalcShapeset ypss(&shapeset);

  // create the x displacement space
  H1Space xdisp(&xmesh, &shapeset);
  xdisp.set_bc_types(bc_types_xy);
  xdisp.set_uniform_order(P_INIT);

  // create the y displacement space
  H1Space ydisp(MULTI ? &ymesh : &xmesh, &shapeset);
  ydisp.set_bc_types(bc_types_xy);
  ydisp.set_uniform_order(P_INIT);

  // enumerate basis functions
  int ndofs = xdisp.assign_dofs();
  ydisp.assign_dofs(ndofs);

  // initialize the weak formulation
  WeakForm wf(2);
  wf.add_biform(0, 0, bilinear_form_0_0, SYM);
  wf.add_biform(0, 1, bilinear_form_0_1, SYM);
  wf.add_biform(1, 1, bilinear_form_1_1, SYM);
  wf.add_liform_surf(1, linear_form_1_surf_top, marker_top);

  // visualization of solution and meshes
  OrderView  xoview("X polynomial orders", 0, 0, 500, 500);
  OrderView  yoview("Y polynomial orders", 510, 0, 500, 500);
  ScalarView sview("Von Mises stress [Pa]", 1020, 0, 500, 500);

  // matrix solver
  UmfpackSolver umfpack;

  // convergence graph wrt. the number of degrees of freedom
  GnuplotGraph graph;
  graph.set_captions("", "Degrees of Freedom", "Error (Energy Norm)");
  graph.set_log_y();
  graph.add_row("Refenrence solution", "k", "-", "O");

  // convergence graph wrt. CPU time
  GnuplotGraph graph_cpu;
  graph_cpu.set_captions("", "CPU", "error");
  graph_cpu.set_log_y();
  graph_cpu.add_row(MULTI ? "multi-mesh" : "single-mesh", "k", "-", "o");

  // adaptivity loop
  int it = 1;
  bool done = false;
  double cpu = 0.0;
  Solution x_sln_coarse, y_sln_coarse;
  Solution x_sln_fine, y_sln_fine;
  do
  {
    info("\n---- Adaptivity step %d ---------------------------------------------\n", it++);

    // time measurement
    begin_time();

    //calculating the number of degrees of freedom
    int ndofs = xdisp.assign_dofs();
    ndofs += ydisp.assign_dofs(ndofs);
    printf("xdof=%d, ydof=%d\n", xdisp.get_num_dofs(), ydisp.get_num_dofs());

    // solve the coarse mesh problem
    LinSystem ls(&wf, &umfpack);
    ls.set_spaces(2, &xdisp, &ydisp);
    ls.set_pss(2, &xpss, &ypss);
    ls.assemble();
    ls.solve(2, &x_sln_coarse, &y_sln_coarse);

    // time measurement
    cpu += end_time();

    // view the solution -- this can be slow; for illustration only
    VonMisesFilter stress_coarse(&x_sln_coarse, &y_sln_coarse, mu, lambda);
    sview.set_min_max_range(0, 3e4);
    sview.show(&stress_coarse);
    xoview.show(&xdisp);
    yoview.show(&ydisp);

    // time measurement
    begin_time();

    // solve the fine mesh problem
    RefSystem rs(&ls);
    rs.assemble();
    rs.solve(2, &x_sln_fine, &y_sln_fine);

    // calculate element errors and total error estimate
    H1OrthoHP hp(2, &xdisp, &ydisp);
    double err_est = hp.calc_energy_error_2(&x_sln_coarse, &y_sln_coarse, &x_sln_fine, &y_sln_fine,
                                          bilinear_form_0_0, bilinear_form_0_1,
                                          bilinear_form_1_0, bilinear_form_1_1) * 100;
    info("Estimate of error: %g%%", err_est);

    // time measurement
    cpu += end_time();

    // add entry to DOF convergence graph
    graph.add_values(0, xdisp.get_num_dofs() + ydisp.get_num_dofs(), err_est);
    graph.save(MULTI ? "conv_dof_m.gp" : "conv_dof_s.gp");

    // add entry to CPU convergence graph
    graph_cpu.add_values(0, cpu, err_est);
    graph_cpu.save(MULTI ? "conv_cpu_m.gp" : "conv_cpu_s.gp");

    // if err_est too large, adapt the mesh
    if (err_est < ERR_STOP) done = true;
    else {
      hp.adapt(THRESHOLD, STRATEGY, ADAPT_TYPE, ISO_ONLY, MESH_REGULARITY, MAX_ORDER, SAME_ORDERS);
      ndofs = xdisp.assign_dofs();
      ndofs += ydisp.assign_dofs(ndofs);
      if (ndofs >= NDOF_STOP) done = true;
    }

    // time measurement
    cpu += end_time();
  }
  while (!done);
  verbose("Total running time: %g sec", cpu);

  // show the fine solution - this is the final result
  VonMisesFilter stress_fine(&x_sln_fine, &y_sln_fine, mu, lambda);
  sview.set_title("Final solution");
  sview.set_min_max_range(0, 3e4);
  sview.show(&stress_fine);

  // wait for keypress or mouse input
  printf("Click into the image window and press 'q' to finish.\n");
  View::wait();
  return 0;
}