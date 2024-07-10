-- target defination, name: 01-hello-triangle
target("01-hello-triangle")
    -- set target kind: executable
    set_kind("binary")

    -- add rules
    add_rules("imguiconfig")

    -- add source files
    add_files("main.cpp")
    add_files("imgui.ini")

    -- add deps
    add_deps("vgfw")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/01-hello-triangle")