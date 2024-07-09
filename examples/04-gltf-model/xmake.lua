-- target defination, name: 04-gltf-model
target("04-gltf-model")
    -- set target kind: executable
    set_kind("binary")

    -- set values
    set_values("asset_files", "assets/models/Suzanne/**")

    -- add rules
    add_rules("copy_assets")

    -- add source files
    add_files("main.cpp")

    -- add deps
    add_deps("vgfw")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/04-gltf-model")