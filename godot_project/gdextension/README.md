# GDExtension Scaffold

This directory is reserved for the native Godot binding layer.

## Phase B

- The native extension is intentionally absent for now so the Godot project can run without trying to load a missing library.
- No native library is built yet.

## Phase C

- Add the real C++ binding sources here.
- Use the `4.6` godot-cpp branch for the dependency checkout or fetch step.
- Export the shared library that implements `chicken_fps_library_init`.
- Wire the compiled library paths into `chicken_fps.gdextension`.
