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

    -- Preprocess shaders (handle #include)
    before_build(function (target)
        -- Helper function to preprocess shaders
        local function preprocess_shaders(shaders_dir, target_dir, extension)
            for _, shader in ipairs(os.files(path.join(shaders_dir, "**." .. extension))) do
                local output_path = path.join(target_dir, path.filename(shader))
                print("Preprocessing shader: " .. shader .. " -> " .. output_path)
                os.execv("glslc", {"-I", shaders_dir, "-E", shader, "-o", output_path})
            end
        end

        local shaders_dir = "$(scriptdir)/shaders"
        local target_shaders_dir = path.join(target:targetdir(), "shaders")

        os.mkdir(target_shaders_dir)

        -- Preprocess vertex, fragment, and geometry shaders
        preprocess_shaders(shaders_dir, target_shaders_dir, "vert")
        preprocess_shaders(shaders_dir, target_shaders_dir, "frag")
        preprocess_shaders(shaders_dir, target_shaders_dir, "geom")
    end)