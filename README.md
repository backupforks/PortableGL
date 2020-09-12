PortableGL
==========

"Because of the nature of Moore's law, anything that an extremely clever graphics programmer can do at one point can be replicated by a merely competent programmer some number of years later." - John Carmack


In a nutshell, PortableGL is an implementation of OpenGL 3.x core in clean C99.  It can theoretically be used with anything that lets you render to a framebuffer/texture but all the demos use SDL2 and it currently only supports 8-bits per channel RGBA as a target (and also for textures).

Its goals are,

1. Portability
2. Matching the API within reason, at the least matching features/abilities
3. Ease of Use
4. Straightforward code
5. Speed

Obviously there are tradeoffs between several of those and some things.  An example where 4 trumps 2 (and arguably 3) is with shaders.  Rather than
write or include a glsl parser and have a built in compiler or interpreter, shaders are just special C functions that match a specific prototype.
Uniforms are another example where 3 and 4 beat 2 because it made no sense to match the API because we can do things so much simpler by just
passing a pointer to a user defined struct (see the examples).

Gallery
=======
![gears](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/gears.png)
![pointsprites](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/pointsprites.png)
![modelviewer](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/modelviewer.png)

History
=======
PortableGL started as a very simple wireframe software renderer based on a tutorial in summer 2011.  I kept playing with it and adding minor features
over the next year and then in early 2013 I decided I should turn it into a software implementation of OpenGL.  This would save me a huge amount of time
and energy on API design since I'd just be implementing an existing good API (though some disagree) and also make the project more useful both to me
and potentially others.  Also, at the time Mesa3D was still years away from full 3.x support, not that I'm really competing but just the fact that there
was no finished implementation was a little motivating.  I made a lot of progress that year and had a few bursts here and there since, but once I got it
mostly working, I was less motivated and when I did work on it I spent my time on creating new demos/examples and tweaking or fixing minor things.  I
could have released an MVP back in 2014 at the earliest but late 2016 would have been the best compromise.  Anyway after somewhere between 300 and 1000+
hours (depending on what/how you count) spread out over 9 years, I've finally finished the the last 90% or at least enough to justify sharing it.
Software is never finished, and I'll be the first to admit PortableGL could use more polish.

Why
===
Aside from the fact that I just wrote it for fun and because I thought it was cool (maybe others will too), I can think of a few semi-practical purposes.

### Educational
I took a 400 level 3D Graphics course in college in fall 2010 months after OpenGL 3.3/4.0 was released.  It was taught based on the original
Red Book using OpenGL 1.1.  Fortunately, the professor let me use OpenGL 3.3 as long as I met the assignment requirements.  Sadly, college graphics
professors still teach 1.x and 2.x OpenGL today in 2020 far more commonly than 3.x/4.x (or Vulkan).  A few are using WebGL 2.0 which I kind of
consider 1 step forward 2 steps back.

While Vulkan is the newest thing (already 4 years old time flies), it really is overkill for learning 3D graphics.  There is rarely anything that students
make in your standard intro to 3D graphics that remotely stresses the performance of any laptop built in the last decade plus.  Using modern OpenGL
to introduce all the standard concepts, vertices, triangles, textures, shaders, fragments/pixels, the transformation pipeline etc. first is much better
than trying to teach them Vulkan and graphics at the same time and obviously better than teaching OpenGL API's that are decades old.

PortableGL could be a very convenient base for such a class.  It's easy to walk through the code and see the pipeline and how all the steps flow together.
For more advanced classes or graduate students in a shared class, modifying PortableGL in someway would be a good project.  It could be some
optimization or algorithm, maybe a new feature.  Theoretically it could be used as a base for actual research into new graphics algorithms or techniques
just because it's such a convenient small base to change and share, vs trying to modify a project the size and complexity of Mesa3D or create a software
renderer from scratch.

### Special Cases
It's hard to imagine any hardware today that has a CPU capable of running software rendered 3D graphics at any respectable speed (especially with full
IEEE floating point) that doesn't *also* have some kind of dedicated GPU.  The GPU might only support OpenGL 2.0 give or take but for performance it'd
be better to stick to whatever the hardware supported than use PortableGL.  However, theoretically, there could be some platform somewhere where the CPU
is relatively powerful that doesn't have GPU.  Maybe some of the current and future RISC SoC's for example?  In such a case PortableGL might be a
useful alternative to Mesa3D or similar.

Another special case is hobby OS's.  The hardware they run on might have a GPU but it might be impossible or more trouble than it's worth to get Mesa3D
to run on some systems.  If they have a C99 compliant compiler and standard library, they could use PortableGL to get at least some OpenGL-ish 3D support.

Documentation
=============
TODO

For now, there's no formal documentation but looking at the examples and demos (and comparing them to [opengl_reference](https://github.com/rswinkle/opengl_reference) should be helpful.  Honestly, the official OpenGL docs and man pages are good for 90-95% of it as far as basic usage.

Building
========
If you have SDL2 installed you should be able to cd into examples, demos, or testing and just run `make` or `make config=release` for an optimized build.
I use premake generated makefiles that I include in the repo, but you should be able to compile it on Windows or Mac too, there's nothing Linux
specific about the code.  I'll fill out this section more later.

Similar/Related Projects
========================
I'll probably add others to this list as I find them.

[TinyGL](https://bellard.org/TinyGL/) is Fabrice Bellard's implementation of a subset of of OpenGL 1.x.  If you want something like PortableGL
but don't want to write shaders, just want old style glBegin/glEnd/glVertex etc. this is the closest I know of.  Also I shamelessly copied his
clipping code because I'm not 1/10th the programmer Bellard was even as an undergrad and I knew it would "just work".  I've included his copyright
and BSD license in LICENSE just in case.


[Pixomatic](http://www.radgametools.com/cn/pixofeat.htm) is/was a software implementation of D3D 7 and 9 written in C and assembly by Michael Abrash
and Mike Sartain.  You can read a [series](https://www.drdobbs.com/architecture-and-design/optimizing-pixomatic-for-x86-processors/184405765)
[of](https://www.drdobbs.com/optimizing-pixomatic-for-modern-x86-proc/184405807)
[articles](https://www.drdobbs.com/optimizing-pixomatic-for-modern-x86-proc/184405848) about it written by Abrash for Dr. Dobbs.


[TTSIOD](https://www.thanassis.space/renderer.html) is an advanced software renderer written in C++.

As an aside, the way I handle interpolation in PortableGL works as a semi-rebuttal of [this article](https://www.thanassis.space/cpp.html).
The answer is not the terrible strawman C approach he comes up with just to easily say "look how bad that is".  The answer is that interpolation is
an algorithm, a simple function, and it doesn't care what the data means or how many elements there are.  Pass it data and let the
algorithm do its job, same as graphics hardware does.  While the inheritance + template functions method works ok if you only have a few "types" of data,
every time you think of some new feature you want to interpolate, you need to define a new struct and a new template function specialization.
Having a function/pipeline that just takes an arbitrary amount of float data to operate on takes less code *and* even has less runtime overhead
since it's a single function that interpolates all the features at once rather than having to call a function for each feature.  See lines ~1200-1250
of gl_internals.c.  Obviously it looks more complicated with all the other openGL stuff going on but you can see a simpler version on line 308 that's
used for interpolating between line endpoints instead of over a triangle.  This is closer to his example but still longer because it has to support
SMOOTH, PERSPECTIVE and FLAT.  You can see the shape of a straightforward implementation even there though, and the benefits of decoupling the
algorithm from the data it operates on.

[Mesa3D](https://mesa3d.org/) is an open source implementation of OpenGL, Vulkan and other graphics APIs.  It includes several different software renderers including the Gallium rasterizer (softpipe or llvmpipe depending on whether llvm is used) and Intel's OpenSWR.

[bbgl](https://github.com/graphitemaster/bbgl) is just a very interesting concept.  When I first saw it soon after it was published I was very frightened that it was exactly what PortableGL is but far more polished and from a better programmer.  Fortunately, it is not.


[pixman](http://pixman.org/) I feel like you could use them together or combine useful parts of pixman with PortableGL.

[fauxgl](https://github.com/fogleman/fauxgl) "3D software rendering in pure Go.  No OpenGL, no C extensions, no nothin'."

LICENSE
=======
PortableGL is licensed under the MIT License (MIT)

The code used for clipping is copyright (c) Fabrice Bellard from TinyGL under the
BSD License, see LICENSE.


TODO/IDEAS
==========
- [ ] Render to texture; do I bother with FBOs/Renderbuffers/PixelBuffers etc.? See ch 8 of superbible 5
- [ ] Multitexture and texture array and render to texture example programs
- [ ] Mapped buffers according to API and...
- [ ] Extension functions that avoid unecessary copying, ie user owns buffer/texture data and gl doesn't free
- [x] Unsafe mode (ie no gl error checking for speedup)
- [ ] Finish duplicating NeHe style tutorial programs from [learningwebgl](https://github.com/rswinkle/webgl-lessons) to [opengl_reference](https://github.com/rswinkle/opengl_reference) and then porting those to use PortableGL
- [ ] Port medium to large open source game project as correctness/performance/API coverage test?
- [ ] Fix cubemapping skybox texture bug
- [ ] More texture and render target formats
- [ ] Formal/organized documentation
- [ ] Integrated documentation, license etc. a la stb libraries

