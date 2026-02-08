# Interactive 3D Museum Scene – OpenGL

## Overview

This project is an interactive real-time 3D scene built using modern OpenGL.
It represents a virtual museum interior featuring architectural elements, artworks, statues, dynamic lighting, shadows, atmospheric effects, and animated objects. The scene is fully navigable using keyboard and mouse input and emphasizes visual realism through advanced rendering techniques.

The application demonstrates real-time rendering concepts such as multi-light illumination, shadow mapping, texture mapping with normal maps, object animation, camera control, and simple collision handling.

---

## Features

### Scene & Interaction

* First-person camera with **keyboard movement (W, A, S, D)** and **mouse look**
* Camera constrained inside the room (no clipping through walls)
* Toggle **wireframe / solid rendering**
* Toggle **flat vs smooth shading**
* Fully modeled indoor environment (floor, walls, ceiling, window)

### Lighting

* **Directional sunlight**
* **Directional window light** with independent shadow mapping
* **Multiple spotlights** illuminating exhibition objects
* Physically inspired attenuation and cutoff angles
* Soft shadows using **PCF (Percentage Closer Filtering)**

### Shadows

* Shadow mapping using depth textures
* Separate shadow maps for:

  * Global light
  * Window light
* Dynamic shadows for static and animated objects

### Materials & Textures

* High-resolution textures
* Normal mapping for surface detail
* Roughness-based material response
* Anisotropic filtering for improved texture quality
* Transparent glass with opacity maps

### Objects & Animation

* Multiple 3D models (.obj) placed on pedestals
* Continuous rotation and vertical bobbing animation for statues
* Animated character:

  * Rotation via arrow keys
  * Forward/backward movement
  * Toggle walking animation
* Decorative wall objects and architectural assets

### Atmosphere

* Distance-based fog
* Volumetric-style window light glow
* Floating dust particles animated in real time

---

## Controls

### Camera

* **W / A / S / D** – Move forward / left / backward / right
* **Mouse** – Look around
* **Q / E** – Rotate selected object
* **F1** – Toggle wireframe mode
* **F2** – Toggle flat shading
* **ESC** – Exit application

### Character

* **Arrow Left / Right** – Rotate character
* **Arrow Up / Down** – Move character
* **Space** – Toggle walking animation

---

## Technologies Used

* **C++**
* **OpenGL (Core Profile)**
* **GLFW** – windowing and input
* **GLEW** – OpenGL extensions
* **GLM** – mathematics library
* **stb_image** – texture loading
* **TinyOBJLoader** – 3D model loading
* **GLSL** – vertex & fragment shaders

## Rendering Pipeline Summary

1. Shadow map generation (directional lights)
2. Main rendering pass with:

   * Multiple light sources
   * Shadow sampling with PCF
   * Material evaluation
3. Transparent pass (glass, dust)
4. Post-effects (fog, light glow)

---

## Notes

* All models are loaded dynamically at runtime
* The scene is designed to be extended with additional rooms, lights, or animations
* The codebase is modular and structured for readability and future expansion

---

## Future Improvements

* Physically Based Rendering (PBR)
* Dynamic global illumination
* Skeletal animation
* Sound and ambient audio
* More complex collision detection


just say the word 👌
