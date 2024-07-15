-- target defination, name: 00-empty-window
target("00-empty-window")
    -- set target kind: executable
    set_kind("binary")

    -- add source files
    add_files("main.cpp")

    -- add deps
    add_deps("vgfw")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/00-empty-window")