-- target defination, name: 03-obj-model
target("03-obj-model")
    -- set target kind: executable
    set_kind("binary")

    -- set values
    set_values("asset_files", "assets/models/spot/**")

    -- add rules
    add_rules("copy_assets", "imguiconfig")

    -- add source files
    add_files("main.cpp")
    add_files("imgui.ini")

    -- add deps
    add_deps("vgfw")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/03-obj-model")