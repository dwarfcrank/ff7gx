"use strict";

const beginEventAddr = Module.findExportByName("apitrace-d3d9.dll", "D3DPERF_BeginEvent");
const endEventAddr = Module.findExportByName("apitrace-d3d9.dll", "D3DPERF_EndEvent");

const D3DPERF_BeginEvent = new NativeFunction(beginEventAddr, "int", ["uint32", "pointer"], "stdcall");
const D3DPERF_EndEvent = new NativeFunction(endEventAddr, "int", [], "stdcall");

const ffviiDllBase = Module.findBaseAddress("AF3DN2.P");
const idaBase = ptr(0x10000000);

function resolveAddr(addr) {
    const offset = ptr(addr).sub(idaBase);
    return ffviiDllBase.add(offset);
}

function beginEvent(name, numArgs, args, returnAddr) {
    let s = name + "(";

    for (let i = 0; i < numArgs; i++) {
        if (i > 0) {
            s += ", ";
        }

        s += "0x" + args[i].toString(16);
    }

    if (returnAddr.compare(ffviiDllBase) > 0) {
        s += ") (return to af3dn+0x" + returnAddr.sub(ffviiDllBase).toString(16) + ")";
    } else {
        s += ") (return to 0x" + returnAddr.toString(16) + ")";
    }

    let buf = Memory.allocUtf16String(s);
    D3DPERF_BeginEvent(0, buf);
}

let functions = {
    GfxFn_0: {
        address: 0x10001850,
        numArgs: 1
    },
    Shutdown: {
        address: 0x100018b0,
        numArgs: 1
    },
    GfxFn_8_C: {
        address: 0x100019b0,
        numArgs: 0
    },
    EndFrame: {
        address: 0x10001cb0,
        numArgs: 1
    },
    Clear: {
        address: 0x10002460,
        numArgs: 2
    },
    ClearAll: {
        address: 0x100025c0,
        numArgs: 0
    },
    SetScissor: {
        address: 0x100025e0,
        numArgs: 4
    },
    SetClearColor: {
        address: 0x10002810,
        numArgs: 1
    },
    GfxFn_40: {
        address: 0x100028b0,
        numArgs: 1
    },
    GfxFn_44: {
        address: 0x10002920,
        numArgs: 0
    },
    GfxFn_48: {
        address: 0x10002930,
        numArgs: 3
    },
    GfxFn_4C: {
        address: 0x100029a0,
        numArgs: 1
    },
    CreateTexture: {
        address: 0x10003380,
        numArgs: 3
    },
    GfxFn_54: {
        address: 0x10003a40,
        numArgs: 5
    },
    GfxFn_58: {
        address: 0x10003ab0,
        numArgs: 6
    },
    GfxFn_5C: {
        address: 0x10003db0,
        numArgs: 1
    },
    SetRenderState: {
        address: 0x10003e00,
        numArgs: 3
    },
    SetupRenderState: {
        address: 0x10003fd0,
        numArgs: 2
    },
    GfxFn_74: {
        address: 0x10004370,
        numArgs: 2
    },
    DrawMesh: {
        address: 0x100043b0,
        numArgs: 2
    },
    GfxFn_7C: {
        address: 0x100043e0,
        numArgs: 2
    },
    GfxFn_80: {
        address: 0x100044d0,
        numArgs: 2
    },
    GfxFn_84: {
        address: 0x10004530,
        numArgs: 2
    },
    GfxFn_88: {
        address: 0x10004640,
        numArgs: 2
    },
    ResetState: {
        address: 0x100046e0,
        numArgs: 1
    },
    GfxFn_90: {
        address: 0x10004760,
        numArgs: 0
    },
    DrawTiles: {
        address: 0x10004ae0,
        numArgs: 2
    },
    DrawTiles2: {
        address: 0x10004c00,
        numArgs: 2
    },
    DrawModel2: {
        address: 0x10004ac0,
        numArgs: 2
    },
    DrawModel3: {
        address: 0x10004be0,
        numArgs: 2
    },
    SetupRenderState2: {
        address: 0x10004a60,
        numArgs: 3
    },
    SetupRenderState3: {
        address: 0x10004b00,
        numArgs: 3
    },
    GfxFn_E4_E8: {
        address: 0x10004c20,
        numArgs: 2
    },
    GfxFn_EC: {
        address: 0x10004c90,
        numArgs: 0
    },

    Draw: {
        address: 0x1000b6a0,
        numArgs: 8
    },
    UseTexture: {
        address: 0x1000ce30,
        numArgs: 1,
        fastcall: true
    },
    SetTexture: {
        address: 0x1000ceb0
    }
};

for (let name in functions) {
    Interceptor.attach(resolveAddr(functions[name].address), {
        onEnter: function (args) {
            if (functions[name].fastcall) {
                args = [this.context.ecx, this.context.edx];
            }

            beginEvent(name, functions[name].numArgs, args, this.returnAddress);
        },

        onLeave: function (args) {
            D3DPERF_EndEvent();
        }
    });
}
