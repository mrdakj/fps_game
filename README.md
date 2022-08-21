# FPS Game

Current scene
![](images/scene_1.png)

Bounding boxes
![](images/bounding_boxes_0.png)

Eye and gun direction tracking
![](images/eye_and_gun_tracking.png)

Pistol animation
![](images/animation.png)

## Description

Project is still work in progress. It is an example of fps game. Goal is to implement the following:
- first person camera :heavy_check_mark:
- animations :heavy_check_mark:
- collisions :heavy_check_mark: (to be improved)
- 3d model mouse selection :heavy_check_mark:
- enemy logic :heavy_check_mark: (to be improved)
- level creation - add more objects :heavy_check_mark: (to be improved)
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

[Swat](https://sketchfab.com/3d-models/swat-32ffa94908434b67b62dda90431b2799)

This work is based on "Swat" (https://sketchfab.com/3d-models/swat-32ffa94908434b67b62dda90431b2799) by Main Light Studios (https://sketchfab.com/sawyerbadoyer) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/).

This project modification: [gun](https://sketchfab.com/3d-models/custom-carbine-rifle-nv4-cod-195d2de670b24e22a98cf9d47607e646) and rotation animation are added for the needs of this project.

[custom_carbine_rifle_nv4_cod](https://sketchfab.com/3d-models/custom-carbine-rifle-nv4-cod-195d2de670b24e22a98cf9d47607e646)

This work is based on "Custom Carbine Rifle NV4 COD" (https://sketchfab.com/3d-models/custom-carbine-rifle-nv4-cod-195d2de670b24e22a98cf9d47607e646) by trolosqlfod (https://sketchfab.com/trolosqlfod) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/).

[Industrial Pipes](https://sketchfab.com/3d-models/industrial-pipes-79f64d09a3cd496fb4f1ae601f5dafac)

This work is based on "Industrial Pipes" (https://sketchfab.com/3d-models/industrial-pipes-79f64d09a3cd496fb4f1ae601f5dafac) by DudleyLong (https://sketchfab.com/DudleyLong) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)

[Industrial Table](https://sketchfab.com/3d-models/industrial-table-2992f30719774ac2b4f89314eaadef74)

This work is based on "Industrial Table" (https://sketchfab.com/3d-models/industrial-table-2992f30719774ac2b4f89314eaadef74) by Logan Thomas (https://sketchfab.com/LThomas762) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)

[wooden box #1](https://sketchfab.com/3d-models/wooden-box-1-ae6a2752a2c14ecfbbef1914b6c00205)

This work is based on "wooden box #1" (https://sketchfab.com/3d-models/wooden-box-1-ae6a2752a2c14ecfbbef1914b6c00205) by DZs (https://sketchfab.com/DZs) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)
