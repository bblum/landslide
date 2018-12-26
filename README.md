# landslide

Landslide is a concurrency testing framework for student thread library and
kernel projects. It uses systematic testing (also known as stateless model
checking) to exhaustively test all thread interleavings possible under a given
test program, or failing that, heuristically prioritizes the most promising
preemption points to find bugs as quickly as possible should they exist.

If you are a current student please make sure to complete the sign-up form
before using Landslide:
- CMU version: http://tinyurl.com/landslide-p2-s18
- PSU version: https://tinyurl.com/landslide-psu-s18

The lecture slides associated with this research are available at:
- CMU version: http://www.cs.cmu.edu/~410/lectures/L14_Landslide.pdf
- PSU version: (available on the internal class website)

Other reading material:
- Paper on hardware transactional memory (SIGBOVIK '18): http://www.contrib.andrew.cmu.edu/~bblum/htm.pdf
- Ph.D. thesis proposal (CMU '17): http://www.contrib.andrew.cmu.edu/~bblum/thesprop.pdf
- Paper on data-race preemption points (OOPSLA '16): http://www.contrib.andrew.cmu.edu/~bblum/oopsla.pdf
- Masters thesis (CMU '12): http://www.contrib.andrew.cmu.edu/~bblum/thesis.pdf

Guide to this repository:
- src/bochs-2.6.8.tar.bz2: unmodified distribution of the Bochs simulator
- src/landslide: Landslide
- src/patches: Interface code between Landslide and Bochs, including a patch to
  edit its API slightly, the Makefile, and the module entrypoint.
- id/: Quicksand (the iterative deepening wrapper described in the OOPSLA paper)
- *-setup.sh: student-friendly initialization scripts for various class projects
- pebsim/: directory for running bochs (config files, annotation scripts)
- pebsim/p2-basecode: directory for importing Pebbles thread library code
- pebsim/pintos: directory for importing Pintos kernel code

## License information

The code in this repository is distributed under mixed licenses according to
the following directory structure:

- Landslide (src/landslide), Quicksand (id/), and all the instrumentation
  scripts (pebsim/*.sh) are distributed under the 3-clause BSD license
  (LICENSE).

- Bochs (src/bochs-2.6.8.tar.bz2) is licensed under the LGPL and distributed in
  its unmodified tarball. Its license is contained therein.

- Interface code between Landslide and Bochs (src/patches/) is subject to LGPL.

- Landslide incorporates the Linux rbtree implementation (src/rbtree) which is
  licensed under GPLv2.

- The P2 basecode (pebsim/p2-basecode/p2-*.tar.bz2) is distributed unlicensed
  with express permission from its copyright holders; please refer to the its
  LICENSE file in the tarball.

Linking Landslide with Bochs and the rbtree produces a result which is subject
to the terms of the GPL to redistribute. For further information please contact
Ben Blum <bblum0@gmail.com> or Dave Eckhardt <David.Eckhardt@cs.cmu.edu>.
