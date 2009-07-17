#include "hermes2d.h"
#include "solver_umfpack.h"

// This example explains how to use the multimesh adaptive hp-FEM,
// where different physical fields (or solution components) can be
// approximated using different meshes and equipped with mutually
// independent adaptivity mechanisms. Here we consider linear elasticity
// and will approximate each displacement components using an individual
// mesh.
//
// PDE: Lame equations of linear elasticity
//
// BC: u_1 = u_2 = 0 on Gamma_1
//     du_2/dn = f on Gamma_2
//     du_1/dn = du_2/dn = 0 elsewhere
//
// The following parameters can be played with:
// (As usual we suggest that you compare hp- and h-adaptivity via the
// H_ONLY option)
//

int P_INIT = 2;           // initial polynomial degree in mesh
double ERR_STOP = 0.01;   // stopping criterion for hp-adaptivity
                          // (rel. error tolerance between the reference
                          // and coarse solution in percent)
double THRESHOLD = 0.3;   // error threshold for element refinement
int STRATEGY = 0;         // refinement strategy (0, 1, 2, 3 - see adapt_h1.cpp for explanation)
int H_ONLY = 0;           // if H_ONLY == 0 then full hp-adaptivity takes place, otherwise
                          // h-adaptivity is used. Use this parameter to check that indeed adaptive
                          // hp-FEM converges much faster than adaptive h-FEM
int NDOF_STOP = 40000;    // adaptivity process stops when the number of degrees of freedom grows over
                          // this limit. This is mainly to prevent h-adaptivity to go on forever.
const double E  = 200e9;  // Young modulus for steel: 200 GPa
const double nu = 0.3;    // Poisson ratio
const double f  = 1e3;    // load force: 10^3 N
const double lambda = (E * nu) / ((1 + nu) * (1 - 2*nu));
const double mu = E / (2*(1 + nu));

const int marker_left = 1;
const int marker_top = 2;


int bc_types_xy(int marker)
  { return (marker == marker_left) ? BC_ESSENTIAL : BC_NATURAL; }


scalar bilinear_form_0_0(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdx_b_dudy_dvdy(lambda+2*mu, fu, mu, fv, ru, rv); }

scalar bilinear_form_0_1(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdy_b_dudy_dvdx(lambda, fv, mu, fu, rv, ru); }

scalar bilinear_form_1_0(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdy_b_dudy_dvdx(lambda, fu, mu, fv, ru, rv); }

scalar bilinear_form_1_1(RealFunction* fu, RealFunction* fv, RefMap* ru, RefMap* rv)
  { return int_a_dudx_dvdx_b_dudy_dvdy(mu, fu, lambda+2*mu, fv, ru, rv); }

scalar linear_form_1_surf_top(RealFunction* fv, RefMap* rv, EdgePos* ep)
  { return -f * surf_int_v(fv, rv, ep); }


int main(int argc, char* argv[])
{
  // load the mesh file
  Mesh xmesh, ymesh;
  xmesh.load("bracket.mesh");
  ymesh.copy(&xmesh);          // this defines the common master mesh for
                               // both displacement fields

  // initialize the shapeset and the cache
  H1Shapeset shapeset;
  PrecalcShapeset xpss(&shapeset);
  PrecalcShapeset ypss(&shapeset);

  // create the x displacement space
  H1Space xdisp(&xmesh, &shapeset);
  xdisp.set_bc_types(bc_types_xy);
  xdisp.set_uniform_order(P_INIT);

  // create the y displacement space
  H1Space ydisp(&ymesh, &shapeset);
  ydisp.set_bc_types(bc_types_xy);
  ydisp.set_uniform_order(P_INIT);

  // initialize the weak formulation
  WeakForm wf(2);
  wf.add_biform(0, 0, bilinear_form_0_0, SYM);
  wf.add_biform(0, 1, bilinear_form_0_1, SYM);
  wf.add_biform(1, 1, bilinear_form_1_1, SYM);
  wf.add_liform_surf(1, linear_form_1_surf_top, marker_top);

  ScalarView sview("Von Mises stress [Pa]", 0, 300, 800, 800);
  OrderView  xoview("X polynomial orders", 0, 0, 800, 800);
  OrderView  yoview("Y polynomial orders", 810, 0, 800, 800);

  GnuplotGraph graph;
  graph.set_captions("Error Convergence", "Degrees of Freedom", "Error [%]");
  graph.add_row("hp-adaptivity", "k", "-", "o");
  graph.set_log_y();

  Solution xsln, ysln;
  Solution xrsln, yrsln;
  UmfpackSolver umfpack;

  char filename[200];

  int it = 1;
  while (1)
  {
    info("\n---- Iteration %d ---------------------------------------------\n", it++);

    //calculating the number of degrees of freedom
    int ndofs = xdisp.assign_dofs();
    ndofs += ydisp.assign_dofs(ndofs);

    // solve the coarse problem
    LinSystem ls(&wf, &umfpack);
    ls.set_spaces(2, &xdisp, &ydisp);
    ls.set_pss(2, &xpss, &ypss);
    ls.assemble();
    ls.solve(2, &xsln, &ysln);

    // visualize the solution
    VonMisesFilter stress(&xsln, &ysln, mu, lambda);
    sview.set_min_max_range(0, 3e4);
    sview.show(&stress);
    xoview.show(&xdisp);
    yoview.show(&ydisp);
    info("");

    // solve the fine (reference) problem
    RefSystem rs(&ls);
    rs.assemble();
    rs.solve(2, &xrsln, &yrsln);

    // calculate errors and adapt the solution
    H1OrthoHP hp(2, &xdisp, &ydisp);
    double error = hp.calc_energy_error_2(&xsln, &ysln, &xrsln, &yrsln,
                                          bilinear_form_0_0, bilinear_form_0_1,
                                          bilinear_form_1_0, bilinear_form_1_1) * 100;
    if (error < ERR_STOP || xdisp.get_num_dofs() + ydisp.get_num_dofs() >= NDOF_STOP) break;
    hp.adapt(THRESHOLD, STRATEGY, H_ONLY);

    graph.add_values(0, xdisp.get_num_dofs() + ydisp.get_num_dofs(), error);
    graph.save("convergence.gp");

  }

  View::wait();
  return 0;
}
