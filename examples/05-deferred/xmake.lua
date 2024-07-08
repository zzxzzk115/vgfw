add_requires("shaderc", {configs = {binaryonly = true}})

-- target defination, name: 05-deferred
target("05-deferred")
    -- set target kind: executable
    set_kind("binary")

    -- add source files
    add_files("main.cpp")

    -- add deps
    add_deps("vgfw")

    -- add packages
    add_packages("shaderc")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/05-deferred")

    -- copy assets
    on_load(function(target)
        os.cp("$(scriptdir)/assets", target:targetdir())
    end)

    -- preprocess shaders (handle #include)
    before_build(function (target)
        local shaders_dir = "$(scriptdir)/shaders"
        
        os.mkdir(path.join(target:targetdir(), "shaders"))

        for _, shader in ipairs(os.files(path.join(shaders_dir, "**.vert"))) do
            print("compiling shader: " .. path.filename(shader))
            os.execv("glslc", {"-I", shaders_dir, "-E", shader, "-o", path.join(target:targetdir(), "shaders", path.filename(shader))})
        end

        for _, shader in ipairs(os.files(path.join(shaders_dir, "**.frag"))) do
            print("compiling shader: " .. path.filename(shader))
            os.execv("glslc", {"-I", shaders_dir, "-E", shader, "-o", path.join(target:targetdir(), "shaders", path.filename(shader))})
        end

        for _, shader in ipairs(os.files(path.join(shaders_dir, "**.geom"))) do
            print("compiling shader: " .. path.filename(shader))
            os.execv("glslc", {"-I", shaders_dir, "-E", shader, "-o", path.join(target:targetdir(), "shaders", path.filename(shader))})
        end
    end)