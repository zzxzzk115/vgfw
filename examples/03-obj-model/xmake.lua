-- target defination, name: 03-obj-model
target("03-obj-model")
    -- set target kind: executable
    set_kind("binary")

    -- add source files
    add_files("main.cpp")

    -- add deps
    add_deps("vgfw")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/03-obj-model")

    -- copy assets
    on_load(function(target)
        os.cp("$(scriptdir)/assets", target:targetdir())
    end)