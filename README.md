# FPS Game

![](images/1.png)

![](images/2.png)

![](images/3.png)

## Description

Project is still work in progress. It is an example of fps game. Goal is to implement the following:
- first person camera :heavy_check_mark:
- animations :heavy_check_mark:
- collisions :heavy_check_mark:
- 3d model mouse selection :heavy_check_mark:
- NPC AI :heavy_check_mark: (to be improved)
- level creation - add more objects :heavy_check_mark: (to be improved)
- sound :heavy_check_mark:

## :package: Installation

:exclamation: Requirements: C++17, cmake, OpenGL, glew, glfw, glm, stb, assimp, imgui

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

[Models licenses](./models_licenses.md)

## Sound

[Sound](https://www.fesliyanstudios.com/sound-effects-search.php?q=)

[Sound](https://www.soundjay.com/gun-sound-effect.html)

## Font

SF Atarian System
