#!/bin/sh
glslangValidator -V model.vert -o model.vert.spv
glslangValidator -V model.frag -o model.frag.spv

glslangValidator -V text.vert -o text.vert.spv
glslangValidator -V text.frag -o text.frag.spv
