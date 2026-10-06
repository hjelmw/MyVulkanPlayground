"""
AnalyzeTerrainCapture.py

Diagnoses the TerrainNode pass in a RenderDoc capture of VulkanEngine and prints a report
with the root causes it can prove from the captured data.

Usage (any Python 3, from the repo root):
    python scripts/AnalyzeTerrainCapture.py "C:/path/to/terrain draw.rdc" [--out DIR] [--qrenderdoc PATH]

RenderDoc for Windows doesn't ship a standalone `renderdoc` Python module, so this script
re-launches itself inside qrenderdoc.exe (--python), whose embedded interpreter has one.
Results are written to <out>/terrain_report.txt (plus a few PNG snapshots) and printed.

What it checks:
    1. Frame structure  - finds the GBuffers / Terrain / Deferred Lighting passes by debug marker
    2. Pipeline state   - topology, culling, winding, render targets of the terrain draws
    3. Matrices         - terrain push constant vs the real camera (GBuffer UBO / lighting UBO)
    4. Vertex data      - grid layout, axis mapping and height range of the terrain vertex buffer
    5. Rasterization    - CPU re-simulation of culling for captured and proposed vertex layouts
    6. Pixels           - what the terrain wrote, and what Deferred Lighting did to it afterwards
    7. Lighting         - whether the terrain would be inside the shadow map once it is lit
"""

import os
import sys

ENV_CAPTURE = "TERRAIN_ANALYSIS_CAPTURE"
ENV_OUTDIR  = "TERRAIN_ANALYSIS_OUTDIR"
REPORT_NAME = "terrain_report.txt"
ERROR_NAME  = "terrain_report_error.txt"

# Values from TerrainNode.cpp, used to decode heights back to heightmap texels
CODE_HEIGHT_SCALE = 64.0
CODE_HEIGHT_SHIFT = 16.0

# Values used to simulate the proposed fix
FIX_HEIGHT_SCALE = 64.0 / 256.0
FIX_HEIGHT_SHIFT = 16.0

try:
    import renderdoc as rd
except ImportError:
    rd = None


# ----------------------------------------------------------------------------------------------
# Small math helpers (no numpy inside qrenderdoc)
# ----------------------------------------------------------------------------------------------

def mat_from_column_major(f):
    """16 floats in glm/GLSL column-major order -> rows, so M[row][col]."""
    return [[f[c * 4 + r] for c in range(4)] for r in range(4)]

def mat_mul(A, B):
    return [[sum(A[r][k] * B[k][c] for k in range(4)) for c in range(4)] for r in range(4)]

def mat_vec(M, v):
    return [M[r][0] * v[0] + M[r][1] * v[1] + M[r][2] * v[2] + M[r][3] * v[3] for r in range(4)]

def dot3(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

def cross3(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]

def len3(a):
    return dot3(a, a) ** 0.5

def norm3(a):
    l = len3(a)
    return [a[0] / l, a[1] / l, a[2] / l] if l > 0 else [0.0, 0.0, 0.0]

def det3(m):
    return (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
          - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
          + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))

def solve3(A, b):
    d = det3(A)
    if abs(d) < 1e-12:
        return None
    out = []
    for c in range(3):
        m = [row[:] for row in A]
        for r in range(3):
            m[r][c] = b[r]
        out.append(det3(m) / d)
    return out

def fv(v, prec=2):
    return "(" + ", ".join(("%." + str(prec) + "f") % x for x in v) + ")"

def fmt_mat(M, indent):
    return [indent + "[" + " ".join("%10.4f" % x for x in row) + " ]" for row in M]

def pct(a, b):
    return 100.0 * a / b if b else 0.0

def decompose_view_projection(M):
    """
    Recover camera parameters from VP = P * V where V is a glm::lookAt and P a glm::perspective
    (rows: r0 = P00*s, r1 = P11*u, r3 = f). P11 < 0 means the projection already flips Y for Vulkan.
    """
    r0, r1, r2, r3 = M
    forward = norm3(r3[:3])
    right   = norm3(r0[:3])
    up      = cross3(right, forward)
    p00     = len3(r0[:3])
    p11     = dot3(r1[:3], up)
    eye     = solve3([r0[:3], r1[:3], r3[:3]], [-r0[3], -r1[3], -r3[3]])
    p22     = -dot3(r2[:3], forward)
    p23     = r2[3] - p22 * dot3(forward, eye) if eye else 0.0
    near    = p23 / p22 if p22 else float("nan")
    far     = p23 / (p22 + 1.0) if (p22 + 1.0) else float("nan")
    import math
    return {
        "eye": eye, "forward": forward, "up": up,
        "p11": p11, "y_flipped": p11 < 0,
        "fovy_deg": math.degrees(2.0 * math.atan(1.0 / abs(p11))) if p11 else float("nan"),
        "aspect": abs(p11) / p00 if p00 else float("nan"),
        "near": near, "far": far,
    }


# ----------------------------------------------------------------------------------------------
# Report
# ----------------------------------------------------------------------------------------------

SEVERITY_ORDER = {"CRITICAL": 0, "HIGH": 1, "MEDIUM": 2, "LOW": 3}

class Report(object):
    def __init__(self):
        self.sections = []
        self.findings = []
        self.images   = []

    def section(self, title):
        lines = []
        self.sections.append((title, lines))
        return lines

    def finding(self, severity, symptom, title, evidence, fix):
        self.findings.append({"severity": severity, "symptom": symptom, "title": title, "evidence": evidence, "fix": fix})

    def render(self, capture_path):
        out = []
        bar = "=" * 100
        out += [bar, " Terrain capture analysis: " + os.path.basename(capture_path), bar, ""]

        findings = sorted(self.findings, key=lambda f: SEVERITY_ORDER.get(f["severity"], 9))
        out.append("FINDINGS (%d)" % len(findings))
        out.append("-" * 100)
        for n, f in enumerate(findings, 1):
            out.append("#%d [%s] (symptom: %s) %s" % (n, f["severity"], f["symptom"], f["title"]))
            for e in f["evidence"]:
                out.append("      evidence: " + e)
            for x in f["fix"]:
                out.append("      fix:      " + x)
            out.append("")

        for title, lines in self.sections:
            out.append("")
            out.append("--- " + title + " " + "-" * max(0, 95 - len(title)))
            out += ["   " + l for l in lines]

        if self.images:
            out.append("")
            out.append("--- Saved images " + "-" * 83)
            out += ["   " + i for i in self.images]
        out.append("")
        return "\n".join(out)


# ----------------------------------------------------------------------------------------------
# RenderDoc helpers (tolerant of API differences between 1.x releases)
# ----------------------------------------------------------------------------------------------

def _ok(result):
    if hasattr(result, "OK"):
        return result.OK()
    return result == rd.ReplayStatus.Succeeded

def _desc(obj):
    return getattr(obj, "descriptor", obj)

def _rid(obj):
    obj = _desc(obj)
    for attr in ("resource", "resourceId"):
        v = getattr(obj, attr, None)
        if v is not None:
            return v
    return None

def _is_null(rid):
    return rid is None or rid == rd.ResourceId.Null()

def _key(rid):
    return str(rid)


class TerrainCaptureAnalysis(object):
    def __init__(self, controller, report, outdir):
        self.ctrl   = controller
        self.rep    = report
        self.outdir = outdir
        self.facts  = {}
        self.sub    = rd.Subresource(0, 0, 0)

    # -- infrastructure -------------------------------------------------------------------------

    def step(self, name, fn):
        try:
            fn()
        except Exception:
            import traceback
            lines = self.rep.section(name + " (FAILED)")
            lines += traceback.format_exc().splitlines()

    def label(self, rid):
        k = _key(rid)
        role = self.roles.get(k)
        name = self.names.get(k, k)
        return "%s [%s]" % (role, name) if role else name

    def pick(self, tex, x, y):
        return list(self.ctrl.PickPixel(tex, x, y, self.sub, rd.CompType.Typeless).floatValue)

    def read_floats(self, rid, offset, size):
        from array import array
        data = self.ctrl.GetBufferData(rid, offset, size)
        a = array("f")
        a.frombytes(bytes(data[: (len(data) // 4) * 4]))
        return a

    def bound_cbuffer(self, pipe, stage):
        refl = pipe.GetShaderReflection(stage)
        for i, cb in enumerate(refl.constantBlocks):
            if not getattr(cb, "bufferBacked", True):
                continue
            d    = _desc(pipe.GetConstantBlock(stage, i, 0))
            rid  = _rid(d)
            off  = getattr(d, "byteOffset", 0)
            size = getattr(d, "byteSize", 0)
            if not _is_null(rid):
                return cb.name, rid, off, size
        return None

    def save_texture(self, rid, eid, filename, black=0.0, white=1.0):
        self.ctrl.SetFrameEvent(eid, True)
        ts = rd.TextureSave()
        ts.resourceId     = rid
        ts.destType       = rd.FileType.PNG
        ts.mip            = 0
        ts.slice.sliceIndex = 0
        ts.alpha          = rd.AlphaMapping.Discard
        ts.comp.blackPoint = black
        ts.comp.whitePoint = white
        path = os.path.join(self.outdir, filename)
        res = self.ctrl.SaveTexture(ts, path)
        if res is None or _ok(res):
            self.rep.images.append("%-38s %s @ event %d" % (filename, self.label(rid), eid))

    # -- analysis -------------------------------------------------------------------------------

    def run(self):
        self.names = {_key(r.resourceId): r.name for r in self.ctrl.GetResources()}
        self.roles = {}
        self.textures = {_key(t.resourceId): t for t in self.ctrl.GetTextures()}

        self.step("Frame structure",            self.find_passes)
        self.step("Terrain pipeline state",     self.pipeline_state)
        self.step("Camera matrices",            self.matrices)
        self.step("Terrain vertex/index data",  self.vertex_data)
        self.step("Rasterization simulation",   self.rasterization)
        self.step("Pixel analysis",             self.pixels)
        self.step("Lighting / shadow coverage", self.lighting)
        self.step("Snapshots",                  self.snapshots)
        self.step("Conclusions",                self.conclusions)

    def find_passes(self):
        sf = self.ctrl.GetStructuredFile()
        passes, order, eid_pass = {}, [], {}

        def walk(actions, current):
            for a in actions:
                name = a.GetName(sf)
                if a.flags & rd.ActionFlags.PushMarker:
                    if name not in passes:
                        passes[name] = []
                        order.append(name)
                    walk(a.children, name)
                    continue
                eid_pass[a.eventId] = current or "-"
                if (a.flags & rd.ActionFlags.Drawcall) and current:
                    passes[current].append(a)
                if len(a.children):
                    walk(a.children, current)

        walk(self.ctrl.GetRootActions(), None)
        self.passes, self.eid_pass = passes, eid_pass

        lines = self.rep.section("Frame structure")
        for name in order:
            draws = passes[name]
            rng = ("events %d..%d" % (draws[0].eventId, draws[-1].eventId)) if draws else "no draws"
            lines.append("%-20s %5d draws  %s" % (name, len(draws), rng))

        for required in ("GBuffers", "Terrain", "Deferred Lighting"):
            if not passes.get(required):
                raise RuntimeError("Could not find draws under the '%s' debug marker" % required)

        self.terrain_draws = passes["Terrain"]
        self.gbuffer_draws = passes["GBuffers"]
        self.light_draw    = passes["Deferred Lighting"][-1]
        ui = passes.get("ImGui Viewport") or []
        self.final_draw    = ui[-1] if ui else None

        t = self.terrain_draws
        counts  = sorted(set(a.numIndices for a in t))
        offsets_ok = all(a.indexOffset == k * t[0].numIndices for k, a in enumerate(t))
        lines.append("")
        lines.append("Terrain: %d vkCmdDrawIndexed calls, numIndices per draw = %s, indexOffset = draw * numIndices: %s"
                     % (len(t), counts, offsets_ok))
        self.facts["terrain_draw_count"] = len(t)
        self.facts["indices_per_draw"]   = t[0].numIndices

    def pipeline_state(self):
        lines = self.rep.section("Terrain pipeline state (first terrain draw, event %d)" % self.terrain_draws[0].eventId)

        # Name the render targets by role using the passes that own them
        self.ctrl.SetFrameEvent(self.gbuffer_draws[0].eventId, True)
        gpipe = self.ctrl.GetPipelineState()
        gouts = [_rid(o) for o in gpipe.GetOutputTargets()]
        for rid, role in zip(gouts, ("GBuffer Positions", "GBuffer Normals", "GBuffer Albedo")):
            if not _is_null(rid):
                self.roles[_key(rid)] = role
        gdepth = _rid(gpipe.GetDepthTarget())
        self.roles[_key(gdepth)] = "Depth"
        self.gbuffer_targets = [r for r in gouts if not _is_null(r)]
        self.depth_target    = gdepth

        self.ctrl.SetFrameEvent(self.light_draw.eventId, True)
        lpipe = self.ctrl.GetPipelineState()
        self.light_targets = [_rid(o) for o in lpipe.GetOutputTargets() if not _is_null(_rid(o))]
        for rid in self.light_targets:
            self.roles[_key(rid)] = "SceneColor"
        self.scene_color = self.light_targets[0]

        if self.final_draw is not None:
            self.ctrl.SetFrameEvent(self.final_draw.eventId, True)
            fo = [_rid(o) for o in self.ctrl.GetPipelineState().GetOutputTargets() if not _is_null(_rid(o))]
            self.backbuffer = fo[0] if fo else None
            if self.backbuffer is not None:
                self.roles.setdefault(_key(self.backbuffer), "Backbuffer")

        # Terrain draw itself
        t0 = self.terrain_draws[0]
        self.ctrl.SetFrameEvent(t0.eventId, True)
        pipe = self.ctrl.GetPipelineState()
        vk   = self.ctrl.GetVulkanPipelineState()

        try:
            topology = pipe.GetPrimitiveTopology()
        except Exception:
            topology = vk.inputAssembly.topology
        rast = vk.rasterizer
        vp   = pipe.GetViewport(0)
        ds   = vk.depthStencil

        self.cull_mode = rast.cullMode
        self.front_ccw = bool(rast.frontCCW)
        self.viewport  = (vp.x, vp.y, vp.width, vp.height)

        self.terrain_targets = [_rid(o) for o in pipe.GetOutputTargets() if not _is_null(_rid(o))]
        self.terrain_depth   = _rid(pipe.GetDepthTarget())

        lines.append("Topology           : %s" % topology)
        lines.append("Cull mode          : %s" % rast.cullMode)
        lines.append("Front face         : %s" % ("counter-clockwise" if self.front_ccw else "clockwise"))
        lines.append("Viewport           : x=%.0f y=%.0f w=%.0f h=%.0f (height %s)"
                     % (vp.x, vp.y, vp.width, vp.height, "negative -> flips Y" if vp.height < 0 else "positive, no flip"))
        lines.append("Depth test/write   : %s / %s, func %s" % (ds.depthTestEnable, ds.depthWriteEnable, ds.depthFunction))
        lines.append("Color targets      : " + ", ".join(self.label(r) for r in self.terrain_targets))
        lines.append("Depth target       : " + self.label(self.terrain_depth))
        lines.append("GBuffer pass writes: " + ", ".join(self.label(r) for r in self.gbuffer_targets))
        lines.append("Lighting pass      : fullscreen draw at event %d writes %s"
                     % (self.light_draw.eventId, ", ".join(self.label(r) for r in self.light_targets)))

        # Vertex inputs / buffers
        self.vb = pipe.GetVBuffers()[0]
        self.ib = pipe.GetIBuffer()
        attrs = pipe.GetVertexInputs()
        for a in attrs:
            lines.append("Vertex attribute   : %s  format=%s offset=%d" % (a.name, a.format.Name(), a.byteOffset))
        lines.append("Vertex buffer      : %s stride=%d" % (self.label(self.vb.resourceId), self.vb.byteStride))
        lines.append("Index buffer       : %s stride=%d (%d-bit)" % (self.label(self.ib.resourceId), self.ib.byteStride, self.ib.byteStride * 8))

        # Shader interface
        vs = pipe.GetShaderReflection(rd.ShaderStage.Vertex)
        fs = pipe.GetShaderReflection(rd.ShaderStage.Fragment)
        lines.append("VS outputs         : " + ", ".join(s.varName for s in vs.outputSignature))
        lines.append("FS outputs         : " + ", ".join("%s -> location %d" % (s.varName, s.regIndex) for s in fs.outputSignature))

        self.push_constants = bytes(vk.pushconsts)
        self.facts["terrain_writes_gbuffer"] = any(_key(r) in set(_key(g) for g in self.gbuffer_targets) for r in self.terrain_targets)
        self.facts["terrain_writes_scenecolor"] = any(_key(r) == _key(self.scene_color) for r in self.terrain_targets)

    def matrices(self):
        import struct
        lines = self.rep.section("Camera matrices")

        # Terrain: mat4 push constant (glm column-major)
        f = struct.unpack("<16f", self.push_constants[:64])
        TVP = mat_from_column_major(f)
        self.terrain_vp = TVP
        tcam = decompose_view_projection(TVP)
        self.terrain_cam = tcam
        lines.append("Terrain push constant m_ViewProjectionMatrix:")
        lines += fmt_mat(TVP, "    ")

        # Validate decoding + detect a Y flip in the shader by comparing against RenderDoc's post-VS output
        self.terrain_shader_negates_y = None
        try:
            from array import array
            t0 = self.terrain_draws[0]
            self.ctrl.SetFrameEvent(t0.eventId, True)
            mesh = self.ctrl.GetPostVSData(0, 0, rd.MeshDataStage.VSOut)
            n = 4
            ib_raw = self.ctrl.GetBufferData(self.ib.resourceId, self.ib.byteOffset + t0.indexOffset * self.ib.byteStride, n * self.ib.byteStride)
            in_idx = array("I" if self.ib.byteStride == 4 else "H")
            in_idx.frombytes(bytes(ib_raw))
            if not _is_null(mesh.indexResourceId):
                raw = self.ctrl.GetBufferData(mesh.indexResourceId, mesh.indexByteOffset, n * mesh.indexByteStride)
                out_idx = array("I" if mesh.indexByteStride == 4 else "H")
                out_idx.frombytes(bytes(raw))
            else:
                out_idx = list(range(n))
            base = getattr(mesh, "baseVertex", 0)
            err_plain, err_flip = 0.0, 0.0
            rows = []
            for k in range(n):
                p = self.read_floats(self.vb.resourceId, self.vb.byteOffset + in_idx[k] * self.vb.byteStride, 12)
                ours = mat_vec(TVP, [p[0], p[1], p[2], 1.0])
                rdoc = list(self.read_floats(mesh.vertexResourceId, mesh.vertexByteOffset + (out_idx[k] + base) * mesh.vertexByteStride, 16))
                err_plain = max(err_plain, max(abs(a - b) for a, b in zip(ours, rdoc)))
                flipped = [ours[0], -ours[1], ours[2], ours[3]]
                err_flip = max(err_flip, max(abs(a - b) for a, b in zip(flipped, rdoc)))
                rows.append("    v%-7d world %-28s VP*v %-36s RenderDoc gl_Position %s" % (in_idx[k], fv(p), fv(ours), fv(rdoc)))
            lines.append("")
            lines.append("Post-VS cross-check (first 4 vertices of draw 0):")
            lines += rows
            scale = max(1.0, max(abs(x) for x in rdoc))
            if err_flip < 1e-3 * scale and err_flip < err_plain:
                self.terrain_shader_negates_y = True
                lines.append("    -> gl_Position == (VP*v) with Y negated: terrain.vert flips Y itself (max err %.2g)" % err_flip)
            elif err_plain < 1e-3 * scale:
                self.terrain_shader_negates_y = False
                lines.append("    -> gl_Position == VP*v: no extra Y flip in the shader (max err %.2g)" % err_plain)
            else:
                lines.append("    -> could not match post-VS output (err %.3g / %.3g)" % (err_plain, err_flip))
        except Exception as e:
            lines.append("Post-VS cross-check skipped: %r" % (e,))

        # Real camera: GBuffer vertex UBO {model, view, proj}
        self.ctrl.SetFrameEvent(self.gbuffer_draws[0].eventId, True)
        gpipe = self.ctrl.GetPipelineState()
        cb = self.bound_cbuffer(gpipe, rd.ShaderStage.Vertex)
        g = self.read_floats(cb[1], cb[2], 192)
        model, view, proj = mat_from_column_major(g[0:16]), mat_from_column_major(g[16:32]), mat_from_column_major(g[32:48])
        CVP = mat_mul(proj, view)
        self.camera_vp = CVP
        ccam = decompose_view_projection(CVP)
        self.camera_cam = ccam
        lines.append("")
        lines.append("Camera from GBuffer UBO '%s' (proj * view):" % cb[0])
        lines += fmt_mat(CVP, "    ")

        # Lighting UBO viewPos is CCamera::GetPosition() directly
        self.ctrl.SetFrameEvent(self.light_draw.eventId, True)
        lpipe = self.ctrl.GetPipelineState()
        lcb = self.bound_cbuffer(lpipe, rd.ShaderStage.Fragment)
        L = self.read_floats(lcb[1], lcb[2], 112)
        self.light = {
            "matrix": mat_from_column_major(L[0:16]), "position": list(L[16:19]), "radius": L[19],
            "color": list(L[20:23]), "intensity": L[23], "viewpos": list(L[24:27]),
        }

        lines.append("")
        lines.append("%-22s %-30s %-30s" % ("", "Terrain push constant", "Engine camera (GBuffer UBO)"))
        lines.append("%-22s %-30s %-30s" % ("eye position", fv(tcam["eye"]), fv(ccam["eye"])))
        lines.append("%-22s %-30s %-30s" % ("forward", fv(tcam["forward"]), fv(ccam["forward"])))
        lines.append("%-22s %-30s %-30s" % ("vertical FOV", "%.1f deg" % tcam["fovy_deg"], "%.1f deg" % ccam["fovy_deg"]))
        lines.append("%-22s %-30s %-30s" % ("near / far", "%.2f / %.0f" % (tcam["near"], tcam["far"]), "%.2f / %.0f" % (ccam["near"], ccam["far"])))
        lines.append("%-22s %-30s %-30s" % ("projection flips Y", tcam["y_flipped"], ccam["y_flipped"]))
        shader_flip = {True: "yes (-position.y)", False: "no", None: "unknown"}[self.terrain_shader_negates_y]
        lines.append("%-22s %-30s %-30s" % ("shader flips Y", shader_flip, "no (geometry.vert)"))
        lines.append("Lighting UBO m_ViewPos (CCamera::GetPosition): %s" % fv(self.light["viewpos"]))

        self.facts["eye_distance"] = len3([a - b for a, b in zip(tcam["eye"], ccam["eye"])])

    def vertex_data(self):
        from array import array
        lines = self.rep.section("Terrain vertex / index data")
        stride = self.vb.byteStride
        data = self.ctrl.GetBufferData(self.vb.resourceId, self.vb.byteOffset, 0)
        nverts = len(data) // stride
        allf = array("f")
        allf.frombytes(bytes(data[: nverts * stride]))
        step = stride // 4
        xs, ys, zs = allf[0::step], allf[1::step], allf[2::step]
        self.xs, self.ys, self.zs, self.nverts = xs, ys, zs, nverts

        # Grid layout: inner loop advances along one axis by 1 unit
        W = len(set(zs)) if abs(zs[1] - zs[0]) > 0 else len(set(xs))
        H = nverts // W
        inner_axis = "z" if abs(zs[1] - zs[0] - 1.0) < 1e-4 else ("x" if abs(xs[1] - xs[0] - 1.0) < 1e-4 else "?")
        outer_axis = "x" if abs(xs[W] - xs[0] - 1.0) < 1e-4 else ("z" if abs(zs[W] - zs[0] - 1.0) < 1e-4 else "?")
        self.W, self.H = W, H

        # Which height scale was the buffer built with? (heights must decode to whole texels)
        sample = ys[:: max(1, nverts // 200000)]
        self.height_scale = None
        for scale in (CODE_HEIGHT_SCALE, FIX_HEIGHT_SCALE):
            if all(abs((y + CODE_HEIGHT_SHIFT) / scale - round((y + CODE_HEIGHT_SHIFT) / scale)) < 1e-3 for y in sample):
                self.height_scale = scale
                break
        ymin, ymax = min(ys), max(ys)

        lines.append("Vertices           : %d (%d x %d grid, %.1f MB)" % (nverts, W, H, len(data) / 1048576.0))
        lines.append("X range            : %.1f .. %.1f  (advances with the OUTER loop -> heightmap rows)" % (min(xs), max(xs))
                     if outer_axis == "x" else "X range            : %.1f .. %.1f" % (min(xs), max(xs)))
        lines.append("Z range            : %.1f .. %.1f  (advances with the INNER loop -> heightmap columns)" % (min(zs), max(zs))
                     if inner_axis == "z" else "Z range            : %.1f .. %.1f" % (min(zs), max(zs)))
        lines.append("Y (height) range   : %.1f .. %.1f" % (ymin, ymax))
        lines.append("Height encoding    : %s" % (("texel * %g - %g" % (self.height_scale, CODE_HEIGHT_SHIFT)) if self.height_scale else "unknown"))
        horiz = max(max(xs) - min(xs), max(zs) - min(zs))
        lines.append("Height span / widest horizontal span = %.1f / %.1f = %.2fx (real Iceland ~0.004x)"
                     % (ymax - ymin, horiz, (ymax - ymin) / horiz))
        eye_y = self.terrain_cam["eye"][1]
        above = sum(1 for y in ys if y > eye_y)
        land  = sum(1 for y in ys if y > ymin)
        lines.append("Vertices above the terrain camera (y=%.1f): %d (%.1f%% of all, %.1f%% of land)"
                     % (eye_y, above, pct(above, nverts), pct(above, land)))
        self.facts.update(height_ratio=(ymax - ymin) / horiz, above_eye_land=pct(above, land), ymax=ymax,
                          axes_transposed=(outer_axis == "x" and inner_axis == "z"))

        # Index buffer
        idata = self.ctrl.GetBufferData(self.ib.resourceId, self.ib.byteOffset, 0)
        idx = array("I" if self.ib.byteStride == 4 else "H")
        idx.frombytes(bytes(idata[: (len(idata) // self.ib.byteStride) * self.ib.byteStride]))
        self.indices = idx
        drawn = len(self.terrain_draws) * self.terrain_draws[0].numIndices
        oob = sum(1 for i in idx if i >= nverts)
        lines.append("")
        lines.append("Indices            : %d in buffer, %d referenced by draws (%d strips x %d)"
                     % (len(idx), drawn, len(self.terrain_draws), self.terrain_draws[0].numIndices))
        lines.append("First indices      : %s  -> (row i, row i+1) zig-zag per column" % list(idx[:6]))
        lines.append("Out-of-range       : %d indices >= vertex count (max index %d, vertex count %d)" % (oob, max(idx), nverts))
        lines.append("                     all of them sit after the last drawn strip: %s" % all(i < nverts for i in idx[:drawn]))
        self.facts["oob_indices"] = oob

    # -- rasterization re-simulation ------------------------------------------------------------

    def simulate(self, pos_of, M, negate_y, strips):
        r0, r1, r2, r3 = M
        eye = decompose_view_projection(M)["eye"]
        vx, vy, vw, vh = self.viewport
        n2 = 2 * self.W
        CN = rd.CullMode
        cull = self.cull_mode
        s = dict(total=0, behind=0, offscreen=0, onscreen=0, culled=0, above_on=0, above_culled=0, below_on=0, below_culled=0,
                 facing_checked=0, facing_agree=0)
        for strip in strips:
            seq = self.indices[strip * n2: strip * n2 + n2]
            P = [pos_of(v) for v in seq]
            S = []
            for (x, y, z) in P:
                cx = r0[0] * x + r0[1] * y + r0[2] * z + r0[3]
                cy = r1[0] * x + r1[1] * y + r1[2] * z + r1[3]
                cz = r2[0] * x + r2[1] * y + r2[2] * z + r2[3]
                cw = r3[0] * x + r3[1] * y + r3[2] * z + r3[3]
                if negate_y:
                    cy = -cy
                if cw <= 1e-6:
                    S.append(None)
                else:
                    S.append((vx + (cx / cw * 0.5 + 0.5) * vw, vy + (cy / cw * 0.5 + 0.5) * vh, cz / cw))
            for t in range(n2 - 2):
                a, b, c = (t, t + 1, t + 2) if t % 2 == 0 else (t, t + 2, t + 1)  # Vulkan strip order
                pa, pb, pc = P[a], P[b], P[c]
                e1 = (pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2])
                e2 = (pc[0] - pa[0], pc[1] - pa[1], pc[2] - pa[2])
                n  = cross3(e1, e2)  # CCW side according to the vertex order
                to_eye = (eye[0] - pa[0], eye[1] - pa[1], eye[2] - pa[2])
                # Is the camera above the surface (on the side of the upward-pointing normal)?
                seen_from_above = dot3(to_eye, n) * (1 if n[1] > 0 else -1) > 0
                s["total"] += 1
                sa, sb, sc = S[a], S[b], S[c]
                if sa is None or sb is None or sc is None:
                    s["behind"] += 1
                    continue
                minx, maxx = min(sa[0], sb[0], sc[0]), max(sa[0], sb[0], sc[0])
                miny, maxy = min(sa[1], sb[1], sc[1]), max(sa[1], sb[1], sc[1])
                minz, maxz = min(sa[2], sb[2], sc[2]), max(sa[2], sb[2], sc[2])
                if maxx < vx or minx > vx + vw or maxy < min(vy, vy + vh) or miny > max(vy, vy + vh) or maxz < 0 or minz > 1:
                    s["offscreen"] += 1
                    continue
                # Vulkan spec: a = -1/2 * sum(x_i*y_i+1 - x_i+1*y_i) in framebuffer coords, a > 0 is CCW
                area = -0.5 * ((sa[0] * sb[1] - sb[0] * sa[1]) + (sb[0] * sc[1] - sc[0] * sb[1]) + (sc[0] * sa[1] - sa[0] * sc[1]))
                front = (area > 0) if self.front_ccw else (area < 0)
                if cull == CN.Back:
                    culled = not front
                elif cull == CN.Front:
                    culled = front
                elif cull == CN.FrontAndBack:
                    culled = True
                else:
                    culled = False
                s["onscreen"] += 1
                s["culled"] += culled
                if n[1] != 0:
                    key = "above" if seen_from_above else "below"
                    s[key + "_on"] += 1
                    s[key + "_culled"] += culled
                # Self-check: screen-space winding must agree with the world-space facing test
                if area != 0:
                    s["facing_checked"] += 1
                    s["facing_agree"] += ((area > 0) == self.front_ccw) == (dot3(to_eye, n) > 0)
        return s

    def rasterization(self):
        lines = self.rep.section("Rasterization re-simulation (CPU, Vulkan rules, %s culling, %s front face)"
                                 % (self.cull_mode, "CCW" if self.front_ccw else "CW"))
        W, H, xs, ys, zs = self.W, self.H, self.xs, self.ys, self.zs
        nstrips = len(self.terrain_draws)
        samples = 48
        strips = sorted(set(int(round(k * (nstrips - 1) / float(samples - 1))) for k in range(samples)))

        def pos_captured(v):
            return (xs[v], ys[v], zs[v])

        # Vertex v is always row i = v / W, column j = v % W; rebuild it with the proposed layout
        height_scale = self.height_scale or CODE_HEIGHT_SCALE

        def pos_fixed(v):
            i, j = divmod(v, W)
            texel = (ys[v] + CODE_HEIGHT_SHIFT) / height_scale
            return (j - W / 2.0, texel * FIX_HEIGHT_SCALE - FIX_HEIGHT_SHIFT, i - H / 2.0)

        self.pos_fixed = pos_fixed
        negate = bool(self.terrain_shader_negates_y)
        scenarios = [
            ("A. As captured (terrain VP%s, current vertices)" % (", shader -y" if negate else ""), pos_captured, self.terrain_vp, negate),
            ("B. Engine camera VP, no shader -y, current vertices",                               pos_captured, self.camera_vp,  False),
            ("C. Engine camera VP, x<-column z<-row, height*0.25",                                pos_fixed,    self.camera_vp,  False),
        ]
        lines.append("Sampled %d of %d strips (%d triangles each)." % (len(strips), nstrips, 2 * W - 2))
        lines.append("")
        lines.append("%-62s %9s %8s %18s %18s %8s" % ("scenario", "on-screen", "culled", "from above:culled", "from below:kept", "check"))
        self.sim = {}
        for name, fn, M, neg in scenarios:
            s = self.simulate(fn, M, neg, strips)
            self.sim[name[0]] = s
            lines.append("%-62s %9d %7.1f%% %8d:%7.1f%% %8d:%7.1f%% %7.1f%%" % (
                name, s["onscreen"], pct(s["culled"], s["onscreen"]),
                s["above_on"], pct(s["above_culled"], s["above_on"]),
                s["below_on"], pct(s["below_on"] - s["below_culled"], s["below_on"]),
                pct(s["facing_agree"], s["facing_checked"])))
        lines.append("")
        lines.append("from above:culled = on-screen triangles whose top side faces the camera, and how many culling throws away")
        lines.append("from below:kept   = on-screen triangles seen from underneath, and how many survive culling")
        lines.append("check             = screen-space winding agrees with the world-space facing test (sanity check)")

    # -- pixels ---------------------------------------------------------------------------------

    def pixels(self):
        lines = self.rep.section("Pixel analysis")
        depth, scene = self.terrain_depth, self.terrain_targets[0]
        tex = self.textures[_key(depth)]
        w, h = tex.width, tex.height
        step = max(8, max(w, h) // 96)
        grid = [(x, y) for y in range(step // 2, h, step) for x in range(step // 2, w, step)]

        gb_last = self.gbuffer_draws[-1].eventId
        t_last  = self.terrain_draws[-1].eventId
        l_eid   = self.light_draw.eventId

        self.ctrl.SetFrameEvent(gb_last, True)
        before = {p: self.pick(depth, p[0], p[1])[0] for p in grid}
        scene_pos = [self.pick(self.gbuffer_targets[0], p[0], p[1])[:3] for p in grid if before[p] < 1.0]
        if scene_pos:
            lo = [min(q[k] for q in scene_pos) for k in range(3)]
            hi = [max(q[k] for q in scene_pos) for k in range(3)]
            self.scene_aabb = (lo, hi)
        self.ctrl.SetFrameEvent(t_last, True)
        after = {p: self.pick(depth, p[0], p[1])[0] for p in grid}
        terrain_px = [p for p in grid if abs(after[p] - before[p]) > 1e-7]
        terrain_col = {p: self.pick(scene, p[0], p[1]) for p in terrain_px}

        lines.append("Sampled %d pixels (every %d px of %dx%d)." % (len(grid), step, w, h))
        lines.append("Terrain changed depth at %d of them (%.1f%% of the screen)." % (len(terrain_px), pct(len(terrain_px), len(grid))))
        self.facts["terrain_coverage"] = pct(len(terrain_px), len(grid))
        d_vals = [after[p] for p in terrain_px]
        self.depth_range = (min(d_vals), max(d_vals)) if d_vals else (0.0, 1.0)
        if not terrain_px:
            return

        self.ctrl.SetFrameEvent(l_eid, True)
        lit = {p: self.pick(self.scene_color, p[0], p[1]) for p in terrain_px}
        gb = {}
        for rid in self.gbuffer_targets:
            gb[self.roles.get(_key(rid), _key(rid))] = {p: self.pick(rid, p[0], p[1]) for p in terrain_px}

        n = len(terrain_px)
        writes_scene = self.facts.get("terrain_writes_scenecolor")
        if writes_scene:
            out_vals = [terrain_col[p][0] for p in terrain_px]
            sat  = sum(1 for v in out_vals if v >= 0.999)
            zero = sum(1 for v in out_vals if v <= 0.001)
            lines.append("")
            lines.append("Terrain fragment output in SceneColor after the Terrain pass:")
            lines.append("    saturated >= 1.0 : %5.1f%%    exactly 0 : %5.1f%%    mean %.2f" % (pct(sat, n), pct(zero, n), sum(out_vals) / n))

        black_after = sum(1 for p in terrain_px if max(abs(c) for c in lit[p][:3]) < 0.02 or any(c != c for c in lit[p][:3]))
        lines.append("Terrain pixels after Deferred Lighting (event %d):" % l_eid)
        lines.append("    black / NaN      : %5.1f%%%s" % (pct(black_after, n), "" if writes_scene else "  (in shadow or facing away from the light)"))
        for role, vals in gb.items():
            cleared = sum(1 for p in terrain_px if all(abs(c) < 1e-6 for c in vals[p]))
            lines.append("    %-17s: %5.1f%% still hold the clear value (0,0,0,0)" % (role, pct(cleared, n)))
            self.facts["cleared_" + role] = pct(cleared, n)
        self.facts["black_after_lighting"] = pct(black_after, n)

        # Representative pixel: a lit-by-terrain pixel that ended up black, nearest the middle of the terrain
        cx = sum(p[0] for p in terrain_px) / float(n)
        cy = sum(p[1] for p in terrain_px) / float(n)
        cands = sorted(terrain_px, key=lambda p: (p[0] - cx) ** 2 + (p[1] - cy) ** 2)
        if writes_scene:
            px = next((p for p in cands if terrain_col[p][0] > 0.5 and max(abs(c) for c in lit[p][:3]) < 0.02), cands[0])
        else:
            px = cands[0]
        self.example_px = px
        lines.append("")
        lines.append("Example pixel %s:" % (px,))
        if writes_scene:
            lines.append("    SceneColor after Terrain : %s" % fv(terrain_col[px], 3))
        lines.append("    SceneColor after Lighting: %s" % fv(lit[px], 3))
        for role, vals in gb.items():
            lines.append("    %-25s: %s" % (role, fv(vals[px], 3)))

        try:
            hist = self.ctrl.PixelHistory(self.scene_color, px[0], px[1], self.sub, rd.CompType.Typeless)
            lines.append("    Pixel history of SceneColor (last fragment per event):")
            last = {}
            for m in hist:
                last[m.eventId] = m
            for eid in sorted(last):
                m = last[eid]
                status = "passed" if m.Passed() else "rejected"
                lines.append("        event %5d  %-18s %-8s  %s -> %s" % (
                    eid, self.eid_pass.get(eid, "?"), status,
                    fv(m.preMod.col.floatValue, 3), fv(m.postMod.col.floatValue, 3)))
        except Exception as e:
            lines.append("    Pixel history skipped: %r" % (e,))

    # -- lighting -------------------------------------------------------------------------------

    def lighting(self):
        lines = self.rep.section("Lighting / shadow coverage")
        L = self.light["matrix"]
        lines.append("Light position %s radius %.0f, shadow light matrix:" % (fv(self.light["position"]), self.light["radius"]))
        lines += fmt_mat(L, "    ")

        def coverage(pos_of):
            inside, total = 0, 0
            for v in range(0, self.nverts, 97):
                p = pos_of(v)
                c = mat_vec(L, [p[0], p[1], p[2], 1.0])
                if c[3] == 0:
                    continue
                x, y, z = c[0] / c[3], c[1] / c[3], c[2] / c[3]
                total += 1
                inside += (-1 <= x <= 1 and -1 <= y <= 1 and 0 <= z <= 1)
            return pct(inside, total)

        cov_fixed = coverage(self.pos_fixed)
        lines.append("Terrain vertices inside the shadow map frustum after the fix: %.1f%%" % cov_fixed)
        lines.append("(deferred.frag multiplies by the shadow term, so texels outside the frustum get an undefined edge lookup)")
        self.facts["shadow_coverage_fixed"] = cov_fixed

        fixed_y = [self.pos_fixed(v)[1] for v in range(0, self.nverts, 97)]
        t_lo = [-self.W / 2.0, min(fixed_y), -self.H / 2.0]
        t_hi = [self.W / 2.0 - 1, max(fixed_y), self.H / 2.0 - 1]
        lines.append("")
        lines.append("Fixed terrain AABB        : %s .. %s" % (fv(t_lo, 1), fv(t_hi, 1)))
        if getattr(self, "scene_aabb", None):
            lo, hi = self.scene_aabb
            lines.append("Visible scene geometry AABB: %s .. %s (from GBuffer positions)" % (fv(lo, 1), fv(hi, 1)))
            overlap = all(lo[k] <= t_hi[k] and hi[k] >= t_lo[k] for k in range(3))
            lines.append("Overlap                    : %s" % ("yes - the models will poke through / sit inside the terrain" if overlap else "no"))
            self.facts["scene_overlap"] = overlap

    def snapshots(self):
        lines = self.rep.section("Snapshots")
        lo, hi = self.depth_range if hasattr(self, "depth_range") else (0.0, 1.0)
        self.save_texture(self.scene_color, self.terrain_draws[-1].eventId, "01_scenecolor_after_terrain.png")
        self.save_texture(self.scene_color, self.light_draw.eventId,        "02_scenecolor_after_lighting.png")
        self.save_texture(self.terrain_depth, self.terrain_draws[-1].eventId, "03_depth_after_terrain.png", lo, hi)
        if len(self.gbuffer_targets) >= 3:
            self.save_texture(self.gbuffer_targets[2], self.light_draw.eventId, "04_gbuffer_albedo.png")
        if getattr(self, "backbuffer", None) is not None:
            self.save_texture(self.backbuffer, self.final_draw.eventId, "05_final_backbuffer.png")
        lines.append("Written to " + self.outdir)

    # -- findings -------------------------------------------------------------------------------

    def conclusions(self):
        F, rep = self.facts, self.rep

        if F.get("terrain_writes_scenecolor") and not F.get("terrain_writes_gbuffer"):
            ev = ["Terrain color target is %s; the GBuffer pass writes %s"
                  % (", ".join(self.label(r) for r in self.terrain_targets), ", ".join(self.label(r) for r in self.gbuffer_targets)),
                  "Deferred Lighting (event %d) draws a fullscreen triangle into SceneColor after the terrain" % self.light_draw.eventId]
            if "black_after_lighting" in F:
                ev.append("%.1f%% of terrain pixels are black after lighting; GBuffer Albedo still holds the clear value at %.1f%% of them"
                          % (F["black_after_lighting"], F.get("cleared_GBuffer Albedo", float("nan"))))
            if getattr(self, "example_px", None):
                ev.append("Pixel history at %s shows the Deferred Lighting event overwriting the terrain's output" % (self.example_px,))
            rep.finding("CRITICAL", "all black", "Terrain is forward-shaded into SceneColor, then Deferred Lighting overwrites it from an empty GBuffer", ev,
                        ["Render the terrain into the GBuffer (Positions, Normals, Albedo + Depth, all LOAD_OP_LOAD) like GeometryNode,",
                         "output world position / normal / albedo from terrain.frag, and give the vertices a normal."])

        if F.get("eye_distance", 0) > 1.0:
            t, c = self.terrain_cam, self.camera_cam
            rep.finding("CRITICAL", "moves with camera", "Terrain uses a hard-coded view-projection instead of the engine camera",
                        ["Terrain VP eye %s fwd %s fov %.0f deg; engine camera eye %s fwd %s fov %.0f deg"
                         % (fv(t["eye"]), fv(t["forward"]), t["fovy_deg"], fv(c["eye"]), fv(c["forward"]), c["fovy_deg"]),
                         "A constant VP pins the terrain to the screen, so it follows every camera movement"],
                        ["Use camera->GetProjectionMatrix() * camera->GetLookAtMatrix() (TerrainNode::Draw)",
                         "and remove the '-position.y' in terrain.vert: CCamera already does proj[1][1] *= -1"])

        sA = self.sim.get("A") if hasattr(self, "sim") else None
        sC = self.sim.get("C") if hasattr(self, "sim") else None
        if sA and pct(sA["above_culled"], sA["above_on"]) > 50:
            fix = ["Map columns to X and rows to Z: x = j - W/2, z = i - H/2 (also un-mirrors Iceland)"]
            if sC:
                fix.append("Re-simulated with all fixes: %.1f%% of triangles seen from above are culled (was %.1f%%)"
                           % (pct(sC["above_culled"], sC["above_on"]), pct(sA["above_culled"], sA["above_on"])))
            rep.finding("CRITICAL", "upside down", "Terrain triangles wind clockwise seen from above, so back-face culling removes the top surface",
                        ["As captured: %.1f%% of on-screen triangles are culled; %.1f%% of those whose top faces the camera are culled,"
                         % (pct(sA["culled"], sA["onscreen"]), pct(sA["above_culled"], sA["above_on"])),
                         "while %.1f%% of those seen from underneath survive" % pct(sA["below_on"] - sA["below_culled"], sA["below_on"]),
                         "Vertices put heightmap rows on X and columns on Z; with indices (i, i+1) that is CW from +Y",
                         "So only the undersides of the (too tall) mountains are drawn -> the terrain looks upside down"],
                        fix)

        if F.get("height_ratio", 0) > 1.0:
            rep.finding("HIGH", "upside down", "Height scale is 256x too large (texel * 64 instead of texel * 64/256)",
                        ["Heights reach %.0f units over a %d x %d grid (%.1fx taller than wide)" % (F["ymax"], self.W, self.H, F["height_ratio"]),
                         "%.1f%% of land vertices are above the terrain camera, so you look at mountains from below" % F["above_eye_land"]],
                        ["terrainHeightScale = 64.0f / 256.0f (LearnOpenGL's yScale), or ~0.05 for true Iceland proportions"])

        if F.get("axes_transposed"):
            rep.finding("MEDIUM", "upside down", "Heightmap is transposed (rows -> X, columns -> Z), so the island is mirrored",
                        ["X advances with the outer row loop, Z with the inner column loop"],
                        ["Same change as the winding fix: x from column j, z from row i"])

        if self.terrain_shader_negates_y and self.camera_cam["y_flipped"]:
            rep.finding("MEDIUM", "upside down", "Y-flip trap: terrain.vert negates gl_Position.y, CCamera's projection already flips Y",
                        ["Today the hard-coded glm::perspective is unflipped, so -y is the only flip. Switching to the camera without",
                         "removing -y would flip twice and render the terrain upside down again"],
                        ["gl_Position = VP * vec4(inPosition, 1.0); (no manual flip), like geometry.vert"])

        if "shadow_coverage_fixed" in F and F["shadow_coverage_fixed"] < 90:
            rep.finding("MEDIUM", "all black (after fix)", "Shadow map frustum is built from model bounds only and misses most of the terrain",
                        ["Only %.1f%% of fixed terrain vertices fall inside the light matrix frustum" % F["shadow_coverage_fixed"]],
                        ["In deferred.frag CalculateShadow, return 1.0 when shadowMapUV is outside [0,1] or z > 1,",
                         "or include the terrain AABB in the scene bounds used by ShadowNode"])

        if F.get("scene_overlap"):
            lo, hi = self.scene_aabb
            rep.finding("LOW", "-", "Terrain (with the proposed layout) intersects the existing scene geometry",
                        ["Scene geometry spans %s .. %s, terrain spans y %.1f .. %.1f over x +-%d, z +-%d"
                         % (fv(lo, 1), fv(hi, 1), -FIX_HEIGHT_SHIFT, 255 * FIX_HEIGHT_SCALE - FIX_HEIGHT_SHIFT, self.W // 2, self.H // 2)],
                        ["Offset or scale the terrain (vertex positions or a model matrix in the push constant) if that isn't intended"])

        if F.get("oob_indices"):
            rep.finding("LOW", "-", "Index buffer has one extra strip pointing past the vertex buffer",
                        ["%d indices >= vertex count (not drawn today because only H-1 strips are issued)" % F["oob_indices"]],
                        ["Loop i < height - 1 when generating indices"])

        if F.get("terrain_draw_count", 0) > 64:
            rep.finding("LOW", "-", "%d draw calls for one mesh" % F["terrain_draw_count"],
                        ["One vkCmdDrawIndexed per strip"],
                        ["Use one draw with primitive restart (0xFFFFFFFF between strips) or degenerate triangles"])


# ----------------------------------------------------------------------------------------------
# Entry points
# ----------------------------------------------------------------------------------------------

def run_inside_renderdoc():
    capture = os.environ[ENV_CAPTURE]
    outdir  = os.environ[ENV_OUTDIR]
    report  = Report()
    cap = rd.OpenCaptureFile()
    try:
        res = cap.OpenFile(capture, "", None)
        if not _ok(res):
            raise RuntimeError("Couldn't open %s: %s" % (capture, res))
        if cap.LocalReplaySupport() != rd.ReplaySupport.Supported:
            raise RuntimeError("Capture can't be replayed on this machine")
        res, controller = cap.OpenCapture(rd.ReplayOptions(), None)
        if not _ok(res):
            raise RuntimeError("Couldn't replay capture: %s" % res)
        try:
            TerrainCaptureAnalysis(controller, report, outdir).run()
        finally:
            controller.Shutdown()
    finally:
        cap.Shutdown()

    with open(os.path.join(outdir, REPORT_NAME), "w") as f:
        f.write(report.render(capture))


def find_qrenderdoc(explicit):
    import shutil
    candidates = [explicit, os.environ.get("QRENDERDOC"), shutil.which("qrenderdoc"),
                  os.path.join(os.environ.get("ProgramFiles", r"C:\Program Files"), "RenderDoc", "qrenderdoc.exe")]
    for c in candidates:
        if c and os.path.isfile(c):
            return c
    sys.exit("qrenderdoc.exe not found, pass --qrenderdoc PATH")


def launch():
    import argparse
    import subprocess
    import time

    parser = argparse.ArgumentParser(description="Diagnose the TerrainNode pass in a RenderDoc capture.")
    parser.add_argument("capture", help="path to the .rdc capture")
    parser.add_argument("--out", help="output directory (default: <capture dir>/terrain_analysis)")
    parser.add_argument("--qrenderdoc", help="path to qrenderdoc.exe")
    parser.add_argument("--timeout", type=int, default=1800, help="seconds before giving up on the replay")
    args = parser.parse_args()

    capture = os.path.abspath(args.capture)
    if not os.path.isfile(capture):
        sys.exit("Capture not found: " + capture)
    outdir = os.path.abspath(args.out or os.path.join(os.path.dirname(capture), "terrain_analysis"))
    os.makedirs(outdir, exist_ok=True)
    for name in (REPORT_NAME, ERROR_NAME):
        if os.path.exists(os.path.join(outdir, name)):
            os.remove(os.path.join(outdir, name))

    env = dict(os.environ)
    env[ENV_CAPTURE] = capture
    env[ENV_OUTDIR]  = outdir

    qrenderdoc = find_qrenderdoc(args.qrenderdoc)
    print("Replaying %s in %s ..." % (capture, qrenderdoc), flush=True)
    start = time.time()
    subprocess.run([qrenderdoc, "--python", os.path.abspath(__file__)], env=env, timeout=args.timeout)
    print("Done in %.0f s\n" % (time.time() - start), flush=True)

    report = os.path.join(outdir, REPORT_NAME)
    error  = os.path.join(outdir, ERROR_NAME)
    if os.path.exists(report):
        with open(report) as f:
            print(f.read())
    if os.path.exists(error):
        with open(error) as f:
            print(f.read())
        sys.exit(1)
    if not os.path.exists(report):
        sys.exit("qrenderdoc exited without writing a report")


if rd is not None and os.environ.get(ENV_CAPTURE):
    # Running inside qrenderdoc: analyse, then hard-exit so the UI never opens
    try:
        run_inside_renderdoc()
    except Exception:
        import traceback
        with open(os.path.join(os.environ.get(ENV_OUTDIR, "."), ERROR_NAME), "w") as f:
            traceback.print_exc(file=f)
    finally:
        os._exit(0)
elif __name__ == "__main__":
    launch()
