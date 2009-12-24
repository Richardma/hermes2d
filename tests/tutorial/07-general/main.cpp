#include "hermes2d.h"
#include "solver_umfpack.h"

// This test makes sure that example 07-general works correctly.
// CAUTION: This test will fail when any changes to the shapeset
// are made, but it is easy to fix (see below).

const int P_INIT = 2;             // Initial polynomial degree of all mesh elements.
const int INIT_REF_NUM = 1;       // Number of initial uniform refinements

// Problem parameters
double a_11(double x, double y) {
  if (y > 0) return 1 + x*x + y*y;
  else return 1;
}

double a_22(double x, double y) {
  if (y > 0) return 1;
  else return 1 + x*x + y*y;
}

double a_12(double x, double y) {
  return 1;
}

double a_21(double x, double y) {
  return 1;
}

double a_1(double x, double y) {
  return 0.0;
}

double a_2(double x, double y) {
  return 0.0;
}

double a_0(double x, double y) {
  return 0.0;
}

double rhs(double x, double y) {
  return 1 + x*x + y*y;
}

double g_D(double x, double y) {
  return -cos(M_PI*x);
}

double g_N(double x, double y) {
  return 0;
}

/********** Boundary conditions ***********/

// Boundary condition types
int bc_types(int marker)
{
  if (marker == 1) return BC_ESSENTIAL;
  else return BC_NATURAL;
}

// Dirichlet boundary condition values
scalar bc_values(int marker, double x, double y)
{
  return g_D(x, y);
}

// (Volumetric) bilinear form
template<typename Real, typename Scalar>
Scalar bilinear_form(int n, double *wt, Func<Real> *u, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
  Scalar result = 0;
  for (int i=0; i < n; i++) {
    double x = e->x[i];
    double y = e->y[i];
    result += (a_11(x, y)*u->dx[i]*v->dx[i] + 
               a_12(x, y)*u->dy[i]*v->dx[i] +
               a_21(x, y)*u->dx[i]*v->dy[i] +
               a_22(x, y)*u->dy[i]*v->dy[i] +
               a_1(x, y)*u->dx[i]*v->val[i] +
               a_2(x, y)*u->dy[i]*v->val[i] +
               a_0(x, y)*u->val[i]*v->val[i]) * wt[i];
  }
  return result;
}

// Integration order for the bilinear form
template<typename Real, typename Scalar>
Scalar bilinear_form_ord(int n, double *wt, Func<Real> *u, 
                         Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
  return u->val[0] * v->val[0] + 2; // returning the sum of the degrees of the basis 
                                    // and test function plus two
}

// Surface linear form (natural boundary conditions)
template<typename Real, typename Scalar>
Scalar linear_form_surf(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
  return int_F_v<Real, Scalar>(n, wt, g_N, v, e);
}

// Integration order for surface linear form
template<typename Real, typename Scalar>
Scalar linear_form_surf_ord(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
  return 2*v->val[0];  // returning twice the polynomial degree of the test function
}

// Volumetric linear form (right-hand side)
template<typename Real, typename Scalar>
Scalar linear_form(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
  return int_F_v<Real, Scalar>(n, wt, rhs, v, e);
}

// Integration order for the volumetric linear form
template<typename Real, typename Scalar>
Scalar linear_form_ord(int n, double *wt, Func<Real> *v, Geom<Real> *e, ExtData<Scalar> *ext)
{
  return 2*v->val[0];  // returning twice the polynomial degree of the test function;
}

int main(int argc, char* argv[])
{
  // Load the mesh
  Mesh mesh;
  mesh.load("domain.mesh");
  for (int i=0; i < INIT_REF_NUM; i++) mesh.refine_all_elements();

  // Initialize the shapeset and the cache
  H1Shapeset shapeset;
  PrecalcShapeset pss(&shapeset);

  // Create finite element space
  H1Space space(&mesh, &shapeset);
  space.set_bc_types(bc_types);
  space.set_bc_values(bc_values);

  // Initialize the weak formulation
  WeakForm wf(1);
  wf.add_biform(0, 0, bilinear_form, bilinear_form_ord, SYM);
  wf.add_liform(0, linear_form, linear_form_ord);
  wf.add_liform_surf(0, linear_form_surf, linear_form_surf_ord, 2);

  // Matrix solver and linear system
  UmfpackSolver solver;
  LinSystem ls(&wf, &solver);
  ls.set_spaces(1, &space);
  ls.set_pss(1, &pss);

  // testing n_dof and correctness of solution vector 
  // for p_init = 1, 2, ..., 10
  int success = 1;
  for (int p_init = 1; p_init <= 10; p_init++) {
    printf("********* p_init = %d *********\n", p_init);
    space.set_uniform_order(p_init);
    space.assign_dofs();

    // Solve the problem
    Solution sln;
    ls.assemble();
    ls.solve(1, &sln);

    scalar *sol_vector;
    int n_dof;
    ls.get_solution_vector(sol_vector, n_dof);
    printf("n_dof = %d\n", n_dof);
    double sum = 0;
    for (int i=0; i < n_dof; i++) sum += sol_vector[i];
    printf("coefficient sum = %g\n", sum);

    // Actual test. The values of 'sum' depend on the 
    // current shapeset. If you change the shapeset, 
    // you need to correct these numbers. 
    if (p_init == 1 && fabs(sum - 1.67824) > 1e-2) success = 0;
    if (p_init == 2 && fabs(sum - 0.295097) > 1e-2) success = 0;
    if (p_init == 3 && fabs(sum - 0.390198) > 1e-2) success = 0;
    if (p_init == 4 && fabs(sum + 0.746589) > 1e-2) success = 0;
    if (p_init == 5 && fabs(sum + 2.62938) > 1e-2) success = 0;
    if (p_init == 6 && fabs(sum + 6.74405) > 1e-2) success = 0;
    if (p_init == 7 && fabs(sum + 17.5057) > 1e-2) success = 0;
    if (p_init == 8 && fabs(sum + 62.7853) > 1e-2) success = 0;
    if (p_init == 9 && fabs(sum - 253.018) > 1e-2) success = 0;
    if (p_init == 10 && fabs(sum - 56.5267) > 1e-2) success = 0;
  }

#define ERROR_SUCCESS                               0
#define ERROR_FAILURE                               -1
  if (success == 1) {
    printf("Success!\n");
    return ERROR_SUCCESS;
  }
  else {
    printf("Failure!\n");
    return ERROR_FAILURE;
  }
}
