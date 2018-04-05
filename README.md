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
- src/bochs-2.6.8: slightly-hacked version of the Bochs simulator
- src/bochs-2.6.8/instrument/landslide: Landslide
- id/: Quicksand (the iterative deepening wrapper described in the OOPSLA paper)
- *-setup.sh: student-friendly initialization scripts for various class projects
- pebsim/: directory for running bochs (config files, annotation scripts)
- pebsim/p2-basecode: directory for importing Pebbles thread library code
- pebsim/pintos: directory for importing Pintos kernel code

Bochs and Landslide are licensed under LGPL. For licensing issues regarding the
P2 and Pintos basecodes (last two bullet points) please contact the OS course
staves at the respective universities.
