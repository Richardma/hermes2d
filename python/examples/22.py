from hermes2d import (Mesh, MeshView, H1Shapeset, PrecalcShapeset, H1Space,
        WeakForm, Solution, DummySolver, LinSystem, ScalarView, RefSystem,
        H1OrthoHP, set_verbose)
from hermes2d.examples.c22 import set_bc, set_forms

threshold = 0.3
strategy = 0
h_only = False
error_tol = 1
interactive_plotting = False    # should the plot be interactively updated
                                # during the calculation? (slower)

set_verbose(False)

mesh = Mesh()
mesh.create([
        [0, 0],
        [1, 0],
        [1, 1],
        [0, 1],
    ], [
        [2, 3, 0, 1, 0],
    ], [
        [0, 1, 1],
        [1, 2, 1],
        [2, 3, 1],
        [3, 0, 1],
    ], [])

mesh.refine_all_elements()

shapeset = H1Shapeset()
pss = PrecalcShapeset(shapeset)

space = H1Space(mesh, shapeset)
set_bc(space)
space.set_uniform_order(1)

wf = WeakForm(1)
set_forms(wf)

sln = Solution()
rsln = Solution()
solver = DummySolver()

view = ScalarView("Solution")
iter = 0
while 1:
    space.assign_dofs()

    sys = LinSystem(wf, solver)
    sys.set_spaces(space)
    sys.set_pss(pss)
    sys.assemble()
    sys.solve_system(sln)
    if interactive_plotting:
        view.show(sln)

    rsys = RefSystem(sys)
    rsys.assemble()

    rsys.solve_system(rsln)

    hp = H1OrthoHP(space)
    error =  hp.calc_error(sln, rsln)*100
    print "iter=%02d, error=%5.2f%%" % (iter, error)
    if error < error_tol:
        break
    hp.adapt(threshold, strategy, h_only)
    iter += 1


if not interactive_plotting:
    view.show(sln)
view.wait()

#mview = MeshView("Mesh")
#mview.show(mesh, lib="mpl")
