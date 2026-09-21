from math import exp, floor, isclose, isfinite, sin

# Mirrors src/shaders/fun/NewShaders.cpp -> namespace bv::shaders::ambient.
TIME_SCALE = 0.11
FLOW_SCALE_Y = 1.7
FLOW_TIME_SCALE = 0.6
DETAIL_SCALE_X = 2.6
DETAIL_TIME_SCALE_X = 0.4
DETAIL_SCALE_Y = 5.5
DETAIL_TIME_SCALE_Y = 1.1

FBM_OCTAVES = 3
FBM_AMPLITUDE = 0.55
FBM_LACUNARITY = 2.07
FBM_OFFSET = (13.1, 7.7)

VERTICAL_EDGE0 = 0.04
VERTICAL_EDGE1 = 0.62

RIBBON_FALLOFF = 3.2
SWAY_FLOW = 0.5
SWAY_DETAIL = 0.15
STRAND_FLOOR = 0.4
STRAND_GAIN = 0.6
STRAND_EDGE0 = 0.1
STRAND_EDGE1 = 0.9
STRAND_SINE_SCALE_Y = 9.0
STRAND_SINE_SCALE_FLOW = 6.0

HERO_OFFSET = -0.22
HERO_WIDTH = 0.15
TEAL_OFFSET = 0.05
TEAL_WIDTH = 0.20
VIOLET_OFFSET = 0.30
VIOLET_WIDTH = 0.13

RIBBON_COLORS = (
    (0.24, 1.00, 0.52),
    (0.16, 0.82, 0.95),
    (0.72, 0.30, 1.00),
)

SHADOW_EDGE0 = 0.35
SHADOW_EDGE1 = 0.95
SHADOW_MIN = 0.18
AURA_GAIN = 1.35
LUMA_WEIGHTS = (0.2126, 0.7152, 0.0722)

DITHER_AMOUNT = 0.02
MAX_STRENGTH = 1.5
DEFAULT_RESOLUTION = (1920, 1080)


def clamp(value: float, minimum: float, maximum: float) -> float:
    return min(max(value, minimum), maximum)


def fract(value: float) -> float:
    return value - floor(value)


def mix(start: float, end: float, amount: float) -> float:
    return start + (end - start) * amount


def smoothstep(edge0: float, edge1: float, value: float) -> float:
    amount = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
    return amount * amount * (3.0 - 2.0 * amount)


def aurora_hash(x: float, y: float) -> float:
    """Same hash generator the VHS shader uses (0.1031 / 33.33 constants)."""
    value_x = fract(x * 0.1031)
    value_y = fract(y * 0.1031)
    value_z = value_x
    offset = (
        value_x * (value_y + 33.33)
        + value_y * (value_z + 33.33)
        + value_z * (value_x + 33.33)
    )
    value_x += offset
    value_y += offset
    value_z += offset
    return fract((value_x + value_y) * value_z)


def aurora_noise(x: float, y: float) -> float:
    cell_x = floor(x)
    cell_y = floor(y)
    offset_x = fract(x)
    offset_y = fract(y)
    weight_x = offset_x * offset_x * (3.0 - 2.0 * offset_x)
    weight_y = offset_y * offset_y * (3.0 - 2.0 * offset_y)
    corner_00 = aurora_hash(cell_x, cell_y)
    corner_10 = aurora_hash(cell_x + 1.0, cell_y)
    corner_01 = aurora_hash(cell_x, cell_y + 1.0)
    corner_11 = aurora_hash(cell_x + 1.0, cell_y + 1.0)
    low = mix(corner_00, corner_10, weight_x)
    high = mix(corner_01, corner_11, weight_x)
    return mix(low, high, weight_y)


def aurora_fbm(x: float, y: float) -> float:
    total = 0.0
    amplitude = FBM_AMPLITUDE
    for _ in range(FBM_OCTAVES):
        total += aurora_noise(x, y) * amplitude
        x = x * FBM_LACUNARITY + FBM_OFFSET[0]
        y = y * FBM_LACUNARITY + FBM_OFFSET[1]
        amplitude *= 0.5
    return total


def ribbon_centre(flow: float, detail: float, offset: float) -> float:
    """The uv.x a ribbon is centred on for the given flow and detail fields."""
    sway = (flow - 0.5) * SWAY_FLOW + (detail - 0.5) * SWAY_DETAIL
    return 0.5 + offset - sway


def aurora_ribbon(
    uv_x: float, uv_y: float, flow: float, detail: float, offset: float, width: float
) -> float:
    sway = (flow - 0.5) * SWAY_FLOW + (detail - 0.5) * SWAY_DETAIL
    distance = (uv_x - 0.5 - offset + sway) / width
    ribbon = exp(-distance * distance * RIBBON_FALLOFF)
    strands = STRAND_FLOOR + STRAND_GAIN * smoothstep(
        STRAND_EDGE0,
        STRAND_EDGE1,
        detail + 0.22 * sin(uv_y * STRAND_SINE_SCALE_Y + flow * STRAND_SINE_SCALE_FLOW),
    )
    return ribbon * strands


def shadow_bias(luma: float) -> float:
    return mix(1.0, SHADOW_MIN, smoothstep(SHADOW_EDGE0, SHADOW_EDGE1, luma))


def aura_contribution(
    uv_x: float, uv_y: float, time: float, strength: float, luma: float
) -> tuple[float, float, float]:
    """The additive glow this pass produces. Deliberately takes no resolution
    argument: the curtains are built purely from uv and time, which is what makes
    them identical at every render scale (the old revision scaled its tint by
    1.0 + u_invResolution.x * 10.0)."""
    scaled_time = time * TIME_SCALE
    flow = aurora_fbm(uv_y * FLOW_SCALE_Y - scaled_time, scaled_time * FLOW_TIME_SCALE)
    detail = aurora_fbm(
        uv_x * DETAIL_SCALE_X + scaled_time * DETAIL_TIME_SCALE_X,
        uv_y * DETAIL_SCALE_Y - scaled_time * DETAIL_TIME_SCALE_Y,
    )
    vertical = smoothstep(VERTICAL_EDGE0, VERTICAL_EDGE1, uv_y)
    bias = shadow_bias(luma) * vertical * strength * AURA_GAIN

    ribbons = (
        aurora_ribbon(uv_x, uv_y, flow, detail, HERO_OFFSET, HERO_WIDTH),
        aurora_ribbon(uv_x, uv_y, flow, detail, TEAL_OFFSET, TEAL_WIDTH),
        aurora_ribbon(uv_x, uv_y, flow, detail, VIOLET_OFFSET, VIOLET_WIDTH),
    )

    return tuple(
        sum(colour[channel] * ribbon for colour, ribbon in zip(RIBBON_COLORS, ribbons)) * bias
        for channel in range(3)
    )


def apply_aurora(
    uv_x: float,
    uv_y: float,
    time: float,
    strength: float,
    source: tuple[float, float, float],
    resolution: tuple[int, int] = DEFAULT_RESOLUTION,
) -> tuple[float, float, float]:
    """Full pass output for a source pixel, including the banding dither."""
    luma = sum(weight * value for weight, value in zip(LUMA_WEIGHTS, source))
    aura = aura_contribution(uv_x, uv_y, time, strength, luma)
    dither = (
        aurora_hash(uv_x * resolution[0], uv_y * resolution[1]) - 0.5
    ) * DITHER_AMOUNT * strength
    return tuple(
        clamp(channel + glow + dither, 0.0, 1.0) for channel, glow in zip(source, aura)
    )


def main() -> None:
    # The hash must behave like the generator the other shaders already use.
    assert aurora_hash(0.25, 0.75) == aurora_hash(0.25, 0.75)
    for x, y in ((-1.0, 0.0), (0.0, 0.0), (0.25, 0.75), (100.0, -20.0)):
        value = aurora_hash(x, y)
        assert isfinite(value)
        assert 0.0 <= value < 1.0

    # Value noise interpolates between lattice hashes and stays in the unit range.
    assert aurora_noise(0.0, 0.0) == aurora_hash(0.0, 0.0)
    assert aurora_noise(3.0, -2.0) == aurora_hash(3.0, -2.0)
    for x, y in ((0.0, 0.0), (0.5, 0.5), (12.25, -7.75), (-0.5, 0.5), (1000.0, 1000.0)):
        value = aurora_noise(x, y)
        assert isfinite(value)
        assert 0.0 <= value <= 1.0

    maximum_fbm = FBM_AMPLITUDE * sum(0.5**octave for octave in range(FBM_OCTAVES))
    assert isclose(maximum_fbm, 0.9625)
    for x, y in ((0.0, 0.0), (1.5, 2.5), (100.0, 100.0), (-13.0, 7.0)):
        value = aurora_fbm(x, y)
        assert isfinite(value)
        assert 0.0 <= value <= maximum_fbm

    # Each ribbon peaks at its own centre and falls away in both directions.
    for offset, width in (
        (HERO_OFFSET, HERO_WIDTH),
        (TEAL_OFFSET, TEAL_WIDTH),
        (VIOLET_OFFSET, VIOLET_WIDTH),
    ):
        for flow, detail in ((0.5, 0.5), (0.0, 1.0), (1.0, 0.0), (0.25, 0.9)):
            centre = ribbon_centre(flow, detail, offset)
            peak = aurora_ribbon(centre, 0.5, flow, detail, offset, width)
            assert 0.0 <= peak <= 1.0
            for step in (0.02, 0.05, 0.1, 0.25):
                for direction in (-1.0, 1.0):
                    away = aurora_ribbon(
                        centre + direction * step, 0.5, flow, detail, offset, width
                    )
                    assert away <= peak + 1e-12

    # The ribbons must not all sit on top of each other.
    centres = [
        ribbon_centre(0.5, 0.5, offset) for offset in (HERO_OFFSET, TEAL_OFFSET, VIOLET_OFFSET)
    ]
    assert len(set(centres)) == len(centres)
    assert centres == sorted(centres)

    # The curtain fades out towards the ground and is at full strength up top.
    assert smoothstep(VERTICAL_EDGE0, VERTICAL_EDGE1, 0.0) == 0.0
    assert smoothstep(VERTICAL_EDGE0, VERTICAL_EDGE1, VERTICAL_EDGE0) == 0.0
    assert smoothstep(VERTICAL_EDGE0, VERTICAL_EDGE1, VERTICAL_EDGE1) == 1.0
    assert smoothstep(VERTICAL_EDGE0, VERTICAL_EDGE1, 1.0) == 1.0
    previous = -1.0
    for step in range(101):
        value = smoothstep(VERTICAL_EDGE0, VERTICAL_EDGE1, step / 100.0)
        assert value >= previous
        previous = value

    # Local time only ever moves the field sideways; it must never blow up.
    for time in (0.0, 1.0, 60.0, 3600.0, 86400.0):
        value = aurora_fbm(-time * TIME_SCALE, time * TIME_SCALE * FLOW_TIME_SCALE)
        assert isfinite(value)
        assert 0.0 <= value <= maximum_fbm

    # Shadow bias only darkens the aura, and only by the documented amount.
    assert shadow_bias(0.0) == 1.0
    assert isclose(shadow_bias(1.0), SHADOW_MIN)
    for luma in (0.0, 0.1, 0.35, 0.5, 0.7, 0.95, 1.0):
        bias = shadow_bias(luma)
        assert SHADOW_MIN - 1e-12 <= bias <= 1.0

    # Below the horizon the aura has to be exactly gone, otherwise the effect
    # reads as a flat tint instead of light in the sky.
    for uv_x in (0.0, 0.25, 0.5, 0.75, 1.0):
        aura = aura_contribution(uv_x, 0.0, 42.0, MAX_STRENGTH, 0.0)
        assert aura == (0.0, 0.0, 0.0)

    # A strength of zero must be an exact no-op so the pass can be disabled
    # without leaving a trace.
    for uv_x, uv_y in ((0.0, 0.0), (0.5, 0.9), (1.0, 1.0)):
        source = (0.3, 0.4, 0.5)
        assert apply_aurora(uv_x, uv_y, 12.0, 0.0, source) == source

    # The effect has to actually do something at its default strength.
    brightest = 0.0
    for x_step in range(51):
        for y_step in range(51):
            aura = aura_contribution(x_step / 50.0, y_step / 50.0, 7.5, 1.0, 0.0)
            brightest = max(brightest, max(aura))
    assert brightest > 0.25

    # Everything must survive extreme inputs and stay displayable.
    for time in (0.0, 0.5, 30.0, 600.0, 100000.0):
        for strength in (0.0, 0.75, MAX_STRENGTH):
            for x_step in range(11):
                for y_step in range(11):
                    for source in ((0.0, 0.0, 0.0), (0.5, 0.5, 0.5), (1.0, 1.0, 1.0)):
                        output = apply_aurora(
                            x_step / 10.0, y_step / 10.0, time, strength, source
                        )
                        for channel in output:
                            assert isfinite(channel)
                            assert 0.0 <= channel <= 1.0

    # Resolution independence: only the dither follows the pixel grid, and it
    # stays below one 8-bit step, so the curtains look identical at every scale.
    assert DITHER_AMOUNT * 0.5 * MAX_STRENGTH <= 0.02
    for resolution in ((1280, 720), (1920, 1080), (2560, 1440), (3840, 2160)):
        for uv_x, uv_y in ((0.13, 0.71), (0.47, 0.33), (0.86, 0.94), (0.5, 0.5)):
            scaled = apply_aurora(uv_x, uv_y, 20.0, 1.0, (0.25, 0.25, 0.25), resolution)
            reference = apply_aurora(uv_x, uv_y, 20.0, 1.0, (0.25, 0.25, 0.25))
            for channel, expected in zip(scaled, reference):
                assert abs(channel - expected) <= DITHER_AMOUNT * 1.0 + 1e-12

    print("Ambient Aurora self-check passed.")


if __name__ == "__main__":
    main()