#!/usr/bin/env python3
"""Static sanity checks for the GLSL embedded in this mod.

The C++ compiler only sees the shader sources as raw string literals, so it
cannot catch GLSL mistakes. OpenGL compiles them at runtime instead, which
means a broken shader silently disables an effect (or worse, logs an error and
bails out of the whole post-process chain).

This script extracts every embedded shader and checks the invariants that
render/PostProcessRenderer.cpp relies on:

  * `u_texture` and `u_invResolution` are always looked up unconditionally, so
    every shader must declare *and use* them. A declared-but-unused uniform is
    removed by the driver and the lookup then fails.
  * every uniform name listed in a `PostProcessShader const kXShader{...}`
    registration must be declared as `uniform float <name>;` in that shader's
    fragment source and must actually be used.
  * braces, parentheses and brackets must balance, string literals must
    terminate, and block comments must be closed.

Usage:  python tools/check_shader_registry.py
Exits non-zero when a check fails.
"""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src"

# `inline constexpr char kFoo[] = R"glsl( ... )glsl";`
RAW_STRING = re.compile(
    r'(?:inline\s+)?(?:constexpr\s+)?(?:char|std::string_view)\s+(\w+)\s*\[\]\s*=\s*R"glsl\((.*?)\)glsl";',
    re.S,
)
# `render::PostProcessShader const kFooShader{ ... };`
REGISTRATION = re.compile(r"PostProcessShader\s+const\s+(k\w+)\s*\{(.*?)\};", re.S)
FLOAT_UNIFORM = re.compile(r"uniform\s+float\s+(\w+)\s*;")
SAMPLER_UNIFORM = re.compile(r"uniform\s+sampler2D\s+(\w+)\s*;")
VEC2_UNIFORM = re.compile(r"uniform\s+vec2\s+(\w+)\s*;")


def split_top_level(text: str) -> list[str]:
    """Split on top-level commas, ignoring commas inside strings or brackets."""
    items: list[str] = []
    current: list[str] = []
    depth = 0
    in_string = False
    i = 0
    while i < len(text):
        ch = text[i]
        if in_string:
            current.append(ch)
            if ch == "\\":
                i += 1
                if i < len(text):
                    current.append(text[i])
            elif ch == '"':
                in_string = False
        elif ch == '"':
            in_string = True
            current.append(ch)
        elif ch in "{([":
            depth += 1
            current.append(ch)
        elif ch in "})]":
            depth -= 1
            current.append(ch)
        elif ch == "," and depth == 0:
            items.append("".join(current).strip())
            current = []
        else:
            current.append(ch)
        i += 1
    tail = "".join(current).strip()
    if tail:
        items.append(tail)
    return items


def unquote(token: str) -> str:
    token = token.strip()
    if token.startswith('"') and token.endswith('"') and len(token) >= 2:
        return token[1:-1]
    return token


def balance_problems(body: str) -> list[str]:
    """Return a list of bracket/string/comment problems in the GLSL body."""
    problems: list[str] = []
    pairs = {"}": "{", ")": "(", "]": "["}
    stack: list[str] = []
    in_string = False
    in_line_comment = False
    in_block_comment = False
    line = 1
    i = 0
    while i < len(body):
        ch = body[i]
        nxt = body[i + 1] if i + 1 < len(body) else ""
        if ch == "\n":
            line += 1
            in_line_comment = False
        if in_line_comment:
            i += 1
            continue
        if in_block_comment:
            if ch == "*" and nxt == "/":
                in_block_comment = False
                i += 2
                continue
            i += 1
            continue
        if in_string:
            if ch == "\\":
                i += 2
                continue
            if ch == '"':
                in_string = False
            i += 1
            continue
        if ch == "/" and nxt == "/":
            in_line_comment = True
            i += 2
            continue
        if ch == "/" and nxt == "*":
            in_block_comment = True
            i += 2
            continue
        if ch == '"':
            in_string = True
            i += 1
            continue
        if ch == "{":
            stack.append(ch)
        elif ch == "}":
            if not stack or stack[-1] != "{":
                problems.append(f"line {line}: unbalanced '}}'")
            else:
                stack.pop()
        elif ch == "(":
            stack.append(ch)
        elif ch == ")":
            if not stack or stack[-1] != "(":
                problems.append(f"line {line}: unbalanced ')'")
            else:
                stack.pop()
        i += 1
    if stack:
        problems.append(f"unclosed '{stack[-1]}' (depth {len(stack)})")
    if in_block_comment:
        problems.append("unterminated block comment")
    if in_string:
        problems.append("unterminated string literal")
    return problems



def rel(path: pathlib.Path) -> str:
    return path.relative_to(ROOT).as_posix()


def collect_sources() -> tuple[dict[str, list[tuple[str, str]]], dict[tuple[str, str], str]]:
    """Index embedded sources globally and by `bv::shaders::<scope>` namespace."""
    global_sources: dict[str, list[tuple[str, str]]] = {}
    scoped_sources: dict[tuple[str, str], str] = {}
    scoped = re.compile(
        r"namespace\s+bv::shaders::(\w+)\s*\{(?:(?!\})[\s\S])*?"
        r"char\s+(\w+)\s*\[\]\s*=\s*R\"glsl\((.*?)\)glsl\";",
        re.S,
    )
    for path in sorted(SRC.rglob("*")):
        if path.suffix not in {".cpp", ".hpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        for name, body in RAW_STRING.findall(text):
            global_sources.setdefault(name, []).append((rel(path), body))
        for scope, name, body in scoped.findall(text):
            scoped_sources[(scope, name)] = body
    return global_sources, scoped_sources


def resolve(
    token: str,
    registration_file: str,
    global_sources: dict[str, list[tuple[str, str]]],
    scoped_sources: dict[tuple[str, str], str],
) -> tuple[str | None, str]:
    """Resolve an initializer token to its embedded source, returning (body, where)."""
    parts = token.split("::")
    name, qualifier = parts[-1], (parts[-2] if len(parts) >= 2 else "")
    if qualifier:
        body = scoped_sources.get((qualifier, name))
        return (body, f"bv::shaders::{qualifier}::{name}") if body is not None else (None, token)
    candidates = global_sources.get(name, [])
    same_file = [c for c in candidates if c[0] == registration_file]
    if same_file:
        return same_file[0][1], same_file[0][0]
    if len(candidates) == 1:
        return candidates[0][1], candidates[0][0]
    if len(candidates) > 1:
        return None, f"{token} (ambiguous: {[c[0] for c in candidates]})"
    return None, token


def check_shader(
    label: str,
    body: str,
    where: str,
    uniforms: list[str],
    problems: list[str],
) -> None:
    for problem in balance_problems(body):
        problems.append(f"{label} [{where}]: {problem}")

    samplers = set(SAMPLER_UNIFORM.findall(body))
    vec2s = set(VEC2_UNIFORM.findall(body))
    floats = set(FLOAT_UNIFORM.findall(body))

    # PostProcessRenderer unconditionally looks these two up.
    for required, declared in (("u_texture", samplers), ("u_invResolution", vec2s)):
        if required not in declared:
            problems.append(f"{label} [{where}]: '{required}' is not declared as a uniform")
        elif len(re.findall(rf"\b{required}\b", body)) < 2:
            problems.append(
                f"{label} [{where}]: '{required}' is declared but never used, so the driver "
                f"strips it and the uniform lookup fails"
            )

    for uniform in uniforms:
        if uniform not in floats:
            problems.append(
                f"{label} [{where}]: registered uniform '{uniform}' is not declared as "
                f"'uniform float'"
            )
        elif len(re.findall(rf"\b{uniform}\b", body)) < 2:
            problems.append(
                f"{label} [{where}]: registered uniform '{uniform}' is declared but never used, "
                f"so the driver strips it and the uniform lookup fails"
            )

    for declared in sorted(floats - set(uniforms)):
        if len(re.findall(rf"\b{declared}\b", body)) < 2:
            problems.append(f"{label} [{where}]: uniform '{declared}' is declared but never used")


def main() -> int:
    global_sources, scoped_sources = collect_sources()
    problems: list[str] = []
    checked = 0
    seen_uniforms: set[str] = set()

    for path in sorted(SRC.rglob("*.cpp")):
        file_name = rel(path)
        text = path.read_text(encoding="utf-8")
        for name, initializer in REGISTRATION.findall(text):
            items = split_top_level(initializer)
            if len(items) < 3:
                problems.append(f"{name} [{file_name}]: fewer than three initializer entries")
                continue

            vertex_name = items[1].split("::")[-1]
            if vertex_name != "kFullscreenVertexSource":
                problems.append(
                    f"{name} [{file_name}]: expected kFullscreenVertexSource, found '{items[1]}'"
                )

            fragment, fragment_where = resolve(items[2], file_name, global_sources, scoped_sources)
            if fragment is None:
                problems.append(f"{name} [{file_name}]: cannot resolve fragment source {fragment_where}")
                continue

            uniforms = [unquote(item) for item in items[3:]]
            uniforms = [u for u in uniforms if u and u != "nullptr"]
            if len(uniforms) > 5:
                problems.append(f"{name} [{file_name}]: {len(uniforms)} uniforms, the renderer only supports 5")

            seen_uniforms.update(uniforms)
            check_shader(name, fragment, fragment_where, uniforms, problems)
            checked += 1

    print(f"checked {checked} PostProcessShader registrations")
    print(f"scalar uniforms in use: {', '.join(sorted(seen_uniforms))}")
    if problems:
        print()
        for problem in problems:
            print(f"FAIL: {problem}")
        return 1
    print("Shader registry check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

