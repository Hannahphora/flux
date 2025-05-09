# Flux Engine

## Building
- compile with gcc or clang

## stuff
- engine.c and all its dependencies get compiled and linked into a single flux.dll engine library
- all allocations to be done through allocators.h

## Common
everything in common is header only
- common
- allocators
- parsing
- assertions
- math
- ds (data structure)
- fs (file system/async io)
- serialisation
- timer

## Core Systems
- engine
- ecs
- logging
- multithreading
- config
- system loading

## Systems
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
- string hashing and ids

## Offline systems
- asset preprocessing
- cache asset location
- unit tests