# cube rain

One of my OpenGL ES 3 based mini project, the goal here is to make rain of cubes with a camera system and diffuse lighting.

![cube rain screenshot](doc/cube_rain.png)



## controls

**space:** stop falling animation, **left mouse:** rotate camera, **right mouse:** pan camera

## building

Building is easy, all we need to do is

```sh
cd cube_rain
scons -j4
```

but before we can do it we need to install *glfw3* and *glew* libraries.

> **Note:** in Ubuntu 19.10 use
>
> ```sh
> sudo apt install libglfw3-dev libglew-dev
> ```
>
> command.

After successful build, `./cube_rain` command can be used to run demo.  
