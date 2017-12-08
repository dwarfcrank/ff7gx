METHOD_WRAPPER_TEMPLATE = """
static {fn.return_type} __cdecl {fn.name}_wrapper({fn.args_decl_str})
{{
    ScopedD3DEvent _(L"{fn.name}({fn.args_fmt_str})"{separator}{fn.call_args_str});
    return FF7::GetGfxFunctions()->rendererInstance->{fn.name}({fn.call_args_str});
}}
"""

METHOD_IMPL_TEMPLATE = """
{fn.return_type} GfxContextBase::{fn.name}({fn.args_decl_str})
{{
    return m_originalImpl->{fn.name}({fn.call_args_str});
}}
"""

METHOD_DECL_TEMPLATE = """virtual {fn.return_type} {fn.name}({fn.args_decl_str});"""

WRAPPER_PTR_TEMPLATE = """/* 0x{fn.offset:02X} */ {fn.return_type} (__cdecl *{fn.name})({fn.args_decl_str});"""
WRAPPER_ASSIGNMENT_TEMPLATE = """m_impl.{fn.name} = {fn.name}_wrapper;"""

CONTEXT_STRUCT_TEMPLATE = """
#pragma once

class GfxContextBase;

namespace FF7
{{
    struct GfxFunctions
    {{
        {fields}
    }};
}}
"""

CLASS_DECL_TEMPLATE = """
#pragma once

class GfxContextBase
{{
public:
    virtual ~GfxContextBase() = default;

    GfxContextBase(FF7::GfxFunctions* impl);
    FF7::GfxFunctions* GetFunctions();

    {methods}

private:
    FF7::GfxFunctions* const m_originalImpl;
    FF7::GfxFunctions m_impl;
}};
"""

CLASS_IMPL_TEMPLATE = """
{method_wrappers}

GfxContextBase::GfxContextBase(FF7::GfxFunctions* impl):
    m_originalImpl(impl), m_impl(*impl)
{{
    m_impl.rendererInstance = this;
    {wrapper_assignments}
}}

FF7::GfxFunctions* GfxContextBase::GetFunctions()
{{
    return &m_impl;
}}

{method_impls}
"""

def is_pointer(arg_type):
    return arg_type[-1] == "*"

class Function:
    def __init__(self, offset, return_type, name, arg_types):
        self.offset = offset
        self.return_type = return_type
        self.name = name
        self.arg_types = arg_types

        self.call_args = map(lambda x: "a{}".format(x), range(0, len(self.arg_types)))
        self.args_decl = map(lambda (x, y): "{} {}".format(x, y), zip(self.arg_types, self.call_args))
        self.args_fmt = map(lambda x: "%p" if is_pointer(x) else "0x%x", self.arg_types)

        self.call_args_str = ", ".join(self.call_args)
        self.args_decl_str = ", ".join(self.args_decl)
        self.args_fmt_str = ", ".join(self.args_fmt)

    def generate_method_wrapper(self):
        separator = ", " if self.call_args else ""
        return METHOD_WRAPPER_TEMPLATE.format(fn=self, separator=separator)

    def generate_method_impl(self):
        separator = ", " if self.call_args else ""
        return METHOD_IMPL_TEMPLATE.format(fn=self, separator=separator)

    def generate_method_decl(self):
        return METHOD_DECL_TEMPLATE.format(fn=self)

    def generate_wrapper_ptr(self):
        return WRAPPER_PTR_TEMPLATE.format(fn=self)

    def generate_wrapper_assignment(self):
        return WRAPPER_ASSIGNMENT_TEMPLATE.format(fn=self)


def generate_context_struct(functions):
    fields = map(lambda x: "/* 0x{offset:02X} */ u32 dword{offset:02X};".format(offset=x), range(0, 0xF0, 4))
    # Hardcoded hack
    fields[0x24 / 4] = "/* 0x24 */ GfxContextBase* rendererInstance;"
    for fn in functions:
        fields[fn.offset / 4] = fn.generate_wrapper_ptr()
    
    return CONTEXT_STRUCT_TEMPLATE.format(fields="\n        ".join(fields))


def generate_class(functions):
    wrapper_assignments = "\n    ".join(map(lambda x: x.generate_wrapper_assignment(), functions))
    method_wrappers = "".join(map(lambda x: x.generate_method_wrapper(), functions))
    method_impls = "".join(map(lambda x: x.generate_method_impl(), functions))
    method_decls = "\n    ".join(map(lambda x: x.generate_method_decl(), functions))
    decl = CLASS_DECL_TEMPLATE.format(methods=method_decls)
    impl = CLASS_IMPL_TEMPLATE.format(method_wrappers=method_wrappers, wrapper_assignments=wrapper_assignments, method_impls=method_impls)
    return (decl, impl)


ff7_gfx_functions = [
    Function(offset=0x00, return_type="u32", name="GfxFn_0", arg_types=["u32"]),
    Function(offset=0x04, return_type="u32", name="Shutdown", arg_types=["u32"]),
    Function(offset=0x10, return_type="u32", name="EndFrame", arg_types=["u32"]),
    Function(offset=0x14, return_type="u32", name="Clear", arg_types=["u32", "u32"]),
    Function(offset=0x18, return_type="u32", name="ClearAll", arg_types=[]),
    Function(offset=0x4C, return_type="u32", name="GfxFn_4C", arg_types=["u32"]),
    Function(offset=0x50, return_type="void*", name="GfxFn_50", arg_types=["void*", "void*", "void*"]),
    Function(offset=0x54, return_type="u32", name="GfxFn_54", arg_types=["u32", "u32", "u32", "u32", "u32"]),
    Function(offset=0x58, return_type="u32", name="GfxFn_58", arg_types=["u32", "u32", "u32", "u32", "u32", "u32"]),
    Function(offset=0x64, return_type="u32", name="SetRenderState", arg_types=["u32", "u32", "u32"]),
    Function(offset=0x68, return_type="u32", name="GfxFn_68", arg_types=["u32", "u32"]),
    Function(offset=0x6C, return_type="u32", name="GfxFn_6C", arg_types=["u32", "u32"]),
    Function(offset=0x70, return_type="u32", name="GfxFn_70", arg_types=["u32", "u32"]),
    Function(offset=0x74, return_type="u32", name="GfxFn_74", arg_types=["u32", "u32"]),
    Function(offset=0x78, return_type="u32", name="GfxFn_78", arg_types=["u32", "u32"]),
    Function(offset=0x7C, return_type="u32", name="GfxFn_7C", arg_types=["u32", "u32"]),
    Function(offset=0x80, return_type="u32", name="GfxFn_80", arg_types=["u32", "u32"]),
    Function(offset=0x84, return_type="void", name="GfxFn_84", arg_types=["u32", "FF7::GameContext*"]),
    Function(offset=0x88, return_type="u32", name="GfxFn_88", arg_types=["u32", "FF7::GameContext*"]),
    Function(offset=0x8C, return_type="u32", name="ResetState", arg_types=["u32"]),
    Function(offset=0xA8, return_type="void", name="DrawModel2_A8", arg_types=["u32", "u32"]),
    Function(offset=0xAC, return_type="void", name="DrawModel2_AC", arg_types=["u32", "u32"]),
    Function(offset=0xB0, return_type="void", name="DrawModel2_B0", arg_types=["u32", "u32"]),
    Function(offset=0xB4, return_type="void", name="DrawTiles", arg_types=["void*", "void*"]),
    Function(offset=0xB8, return_type="u32", name="GfxFn_B8", arg_types=["u32", "u32", "u32"]),
    Function(offset=0xBC, return_type="u32", name="GfxFn_BC", arg_types=["u32", "u32", "u32"]),
    Function(offset=0xC0, return_type="u32", name="GfxFn_C0", arg_types=["u32", "u32", "u32"]),
    Function(offset=0xC4, return_type="u32", name="GfxFn_C4", arg_types=["u32", "u32", "u32"]),
    Function(offset=0xC8, return_type="u32", name="GfxFn_C8", arg_types=["u32", "u32", "u32"]),
    Function(offset=0xCC, return_type="void", name="DrawModel3_CC", arg_types=["u32", "u32"]),
    Function(offset=0xD0, return_type="void", name="DrawModel3_D0", arg_types=["u32", "u32"]),
    Function(offset=0xD4, return_type="void", name="DrawModel3_D4", arg_types=["u32", "u32"]),
    Function(offset=0xD8, return_type="void", name="DrawTiles2", arg_types=["void*", "void*"]),
    Function(offset=0xE4, return_type="u32", name="GfxFn_E4", arg_types=["u32", "u32"]),
    Function(offset=0xE8, return_type="u32", name="GfxFn_E8", arg_types=["u32", "u32"]),
]

decl, impl = generate_class(ff7_gfx_functions)
ctx = generate_context_struct(ff7_gfx_functions)

with open("Generated/GfxContextBase.h", "w") as f:
    f.write(decl)

with open("Generated/GfxContextBase.cpp", "w") as f:
    f.write(impl)

with open("Generated/GfxFunctions.h", "w") as f:
    f.write(ctx)