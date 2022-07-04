# FPS Game

Current scene
![](images/current.png)

Bounding boxes
![](images/bounding_boxes.png)

Eye and gun direction tracking
![](images/eye_and_gun_tracking.png)

Pistol animation
![](images/animation.png)

## Description

Project is still work in progress. It is an example of fps game. Goal is to implement the following:
- first person camera :heavy_check_mark:
- animations :heavy_check_mark: (more to be added)
- collisions :heavy_check_mark: (efficiency to be improved using some space division)
- 3d model mouse selection :heavy_check_mark:
- enemy logic (to be done, started)
- scene creation - add more objects (to be done)
- sound (to be done)

## :package: Installation

:exclamation: Requirements: C++17, cmake, OpenGL, glew, glfw, glm, stb, assimp

To install glew:

```sh
pacman -S glew

```

To install glfw:

```sh
pacman -S glfw

```

To install glm:

```sh
pacman -S glm

```

To install stb from AUR (single-file public domain (or MIT licensed) libraries for C/C++):

```sh
yay -S stb

```


## Usage

Compile:

```sh
mkdir build
cd build
cmake ..
(cmake -DCMAKE_BUILD_TYPE=Debug  ..)
cmake --build .

```

Run:

```sh
./src/main
```

## 3D Models

[fps-pistol](https://sketchfab.com/3d-models/fps-pistol-animated-e00b3155ce484eb9aae2ff23a333342d)

This work is based on "Fps pistol animated" (https://sketchfab.com/3d-models/fps-pistol-animated-e00b3155ce484eb9aae2ff23a333342d) by wout.uyttebroek (https://sketchfab.com/wout.uyttebroek) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)

This project modification: one animation separated, reduced number of polygons

[Swat](https://sketchfab.com/3d-models/swat-32ffa94908434b67b62dda90431b2799)

This work is based on "Swat" (https://sketchfab.com/3d-models/swat-32ffa94908434b67b62dda90431b2799) by Main Light Studios (https://sketchfab.com/sawyerbadoyer) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/).

This project modification: [gun](https://sketchfab.com/3d-models/custom-carbine-rifle-nv4-cod-195d2de670b24e22a98cf9d47607e646) and rotation animation are added for the needs of this project, reduced number of polygons

[custom_carbine_rifle_nv4_cod](https://sketchfab.com/3d-models/custom-carbine-rifle-nv4-cod-195d2de670b24e22a98cf9d47607e646)

This work is based on "Custom Carbine Rifle NV4 COD" (https://sketchfab.com/3d-models/custom-carbine-rifle-nv4-cod-195d2de670b24e22a98cf9d47607e646) by trolosqlfod (https://sketchfab.com/trolosqlfod) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/).

This project modification: reduced number of polygons
