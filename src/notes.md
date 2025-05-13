# Flux Engine

## Building
- bootstrap build system with gcc -o build build.c
- 

## Structure

### Common
everything in common is header only
- common
- allocators
- parsing
- assertions
- math
- ds (data structures)
- fs (file system/async io)
- serialisation
- timer

### Core Systems
- engine
- ecs
- logging
- multithreading
- config
- system loading

### Systems
- rendering
- ai
- animation
- audio
- gameplay/scripting
- input
- networking
- physics
- ui
- scene graph/management

### Offline systems
- asset preprocessing
- cache asset location
- unit testing

## other stuff

- engine.c and all its dependencies get compiled and linked into a single flux.dll engine library
- all allocations to be done through allocators.h
- target hardware: x86_64 cpus, assuming stuff like 4kb page size, 64b cache lines etc...