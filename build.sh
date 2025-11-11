#!/bin/bash

mkdir -p build/

gcc ./src/main.c -g -o ./build/raytracer -lm
