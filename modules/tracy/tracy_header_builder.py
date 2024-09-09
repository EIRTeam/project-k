def make_tracy_header(target, source, env):
    defines = env["CPPDEFINES"]

    g = open(str(target[0]), "w", encoding="utf-8")
    g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    g.write("#ifndef GODOT_TRACY_H\n")
    g.write("#define GODOT_TRACY_H\n\n")
    for d in defines:
        if isinstance(d, str):
            if d.startswith("TRACY_"):
                g.write("#define " + d + "\n")
    g.write("\n")
    g.write('#include "thirdparty/tracy/public/tracy/Tracy.hpp"\n\n')

    g.write("#endif // GODOT_TRACY_H\n")
    g.close()
