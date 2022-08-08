#!/usr/bin/env python3

import licant
import licant.install

licant.include("rabbit")

licant.cxx_application("modvisio",
                       sources=["src/main.cpp"],
                       libs=["GL", "GLEW", "glfw", "igris", "ralgo", "nos", "crow", "GLU"],
                       mdepends=["rabbit", "rabbit.opengl"]
                       )

licant.install.install_application(tgt="install", dst="modvisio", src="modvisio")

licant.ex("modvisio")
