add_requires("glslang", {configs = {binaryonly = true}})

-- target defination, name: 01-hello-triangle
target("01-hello-triangle")
    -- set target kind: executable
    set_kind("binary")

    -- add source files
    add_files("main.cpp")

    -- add shader files
    add_files("**.vert", "**.frag")

    -- add deps
    add_deps("vgfw")

    -- add packages
    add_packages("glslang")

    -- set target directory
    local target_dir = "$(buildir)/$(plat)/$(arch)/$(mode)/examples/01-hello-triangle/"
    set_targetdir(target_dir)

    -- convert glsl shaders to spv
    add_rules("utils.glsl2spv", {outputdir = target_dir .. "assets/shaders"})