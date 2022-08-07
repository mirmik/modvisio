#!/usr/bin/env python3

import licant

licant.include("rabbit")

licant.cxx_application("modvisio",
                       sources=["src/main.cpp"],
                       libs=["GL", "GLEW", "glfw", "igris", "ralgo", "nos", "crow", "GLU"],
                       mdepends=["rabbit", "rabbit.opengl"]
                       )

licant.ex("modvisio")
