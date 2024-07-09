-- target defination, name: 02-cube
target("02-cube")
    -- set target kind: executable
    set_kind("binary")

    -- set values
    set_values("asset_files", "assets/textures/awesomeface.png")

    -- add rules
    add_rules("copy_assets")
    
    -- add source files
    add_files("main.cpp")

    -- add deps
    add_deps("vgfw")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/02-cube")