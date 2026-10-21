
add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("client")
    set_showmenu(true)
    set_values("server", "client")
option_end()

package("mossui")
    set_homepage("https://github.com/Kashifro/MossUI")
    set_description("Cross-platform C++ UI library")
    set_license("MIT")
    set_kind("library")
    add_urls("https://github.com/Kashifro/MossUI.git")
    add_versions("1.0.0", "f36eaa350fd312029935c99f99726d5e2b38045e")
    add_includedirs("include")
    add_linkdirs("lib")
    add_links("MossUI")
    on_install(function (package)
        import("package.tools.cmake").install(package, {"-DMOSSUI_BUILD_TESTS=OFF"})
    end)
package_end()

add_requires("levilamina 26.51.7", {
    configs = {
        target_type = get_config("target_type")
    }
})

add_requires("levibuildscript")
add_requires("mossui 1.0.0")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("ModMenu")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker", {modVersion = "1.0.0"})

    if is_plat("windows") then
        add_defines(
            "NOMINMAX",
            "UNICODE",
            "_HAS_CXX23=1",
            "WIN32_LEAN_AND_MEAN",
            "MODMENU_BUILD"
        )

        set_exceptions("none")

        add_cxflags(
            "/EHa", "/utf-8", "/W4", "/w44265", "/w44289",
            "/w44296", "/w45263", "/w44738", "/w45204"
        )

        add_cxflags(
            "/EHs",
            "-Wno-microsoft-cast",
            "-Wno-invalid-offsetof",
            "-Wno-c++2b-extensions",
            "-Wno-microsoft-include",
            "-Wno-overloaded-virtual",
            "-Wno-ignored-qualifiers",
            "-Wno-missing-field-initializers",
            "-Wno-potentially-evaluated-expression",
            "-Wno-pragma-system-header-outside-header",
            { tools = {"clang-cl"} }
        )

        set_toolchains("clang-cl")
        add_syslinks("user32", "shell32", "advapi32")
    end

    add_packages("levilamina", "mossui")

    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")

    add_headerfiles("src/**.h", "src/**.hpp")
    add_files("src/**.cpp")
    add_includedirs("src")

    before_build(function (target)
        local manifest_path = path.join(os.projectdir(), "manifest.json")
        local manifest = io.readfile(manifest_path)
        if not manifest then
            os.raise("LL-ModMenu manifest.json is missing")
        end

        local platform = manifest:match('"platform"%s*:%s*"([^"]+)"')
        local target_type = get_config("target_type")
        if not platform then
            os.raise("LL-ModMenu manifest.json must declare a platform")
        end
        if platform ~= target_type then
            os.raise("LL-ModMenu manifest platform '%s' does not match target_type '%s'", platform, target_type)
        end
    end)

    after_build(function (target)
        local outputdir = path.join(os.projectdir(), "bin", target:name())
        local packRoot = path.join(outputdir, "resource_packs")
        local packdir = path.join(packRoot, "ModMenu")
        local oldPackDir = path.join(outputdir, "resources")
        local assetdir = path.join(outputdir, "assets")
        os.rm(oldPackDir)
        os.rm(packRoot)
        os.rm(assetdir)
        os.mkdir(packdir)
        os.mkdir(assetdir)
        os.cp(path.join(os.projectdir(), "resource_packs", "*"), packdir)
        os.cp(path.join(os.projectdir(), "assets", "*"), assetdir)
        local icon = path.join(os.projectdir(), "icon.png")
        if os.isfile(icon) then
            os.cp(icon, outputdir)
        end
    end)

