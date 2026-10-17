"""Character manifest validation and SDK header generation. No game or build state."""

import json
import math
import re

FLAGS = {"automatic": 1, "radial": 2, "no_objects": 4}
CAPS = {
    "audio": 1,
    "item_prop": 2,
    "texture_modulate": 4,
    "alpha_test": 8,
    "equipment": 16,
    "death_pose": 32,
    "melee_families": 64,
    "native_combat": 128,
    "pyromancy": 256,
    "native_walls": 512,
}
BODY_DEFAULTS = dict(
    model_scale=1.0,
    radius_cm=0.0,
    height_cm=0.0,
    hurt_radius_cm=0.0,
    hurt_height_cm=0.0,
    fall_damage_scale=1.0,
    fall_damage_max_hp_fraction=1.0,
    nonlethal_falls=False,
)


def load_profile(character, characters_directory):
    """Read one manifest, normalize defaults, and enforce the SDK boundary."""
    if not re.fullmatch("[a-z][a-z0-9_]{0,46}", character):
        raise ValueError("Use a lowercase character identifier")
    folder = characters_directory / character
    profile = json.loads((folder / "character.json").read_text(encoding="utf-8"))
    if profile["schema"] != 1 or profile["id"] != character:
        raise ValueError("Manifest version/identity mismatch")
    if not re.fullmatch("[a-zA-Z0-9_./-]+", profile["source"]) or not (
        folder / profile["source"]
    ).resolve().is_relative_to(folder.resolve()):
        raise ValueError("Adapter source must stay inside its character folder")
    if not (folder / profile["source"]).is_file():
        raise ValueError("Adapter source is missing")
    if (
        len(profile["attacks"]) > 32
        or not 1 <= profile["max_triangles"] <= 16384
        or len(profile["texture"]) != 2
        or not all(1 <= v <= 4096 for v in profile["texture"])
    ):
        raise ValueError("Profile exceeds SDK bounds")
    body = BODY_DEFAULTS | profile.get("body", {})
    if set(body) != set(BODY_DEFAULTS):
        raise ValueError("Unknown body setting")
    if not isinstance(body["nonlethal_falls"], bool):
        raise ValueError("nonlethal_falls must be boolean")
    for key, value in body.items():
        if key != "nonlethal_falls" and (
            not isinstance(value, (int, float))
            or isinstance(value, bool)
            or not math.isfinite(value)
        ):
            raise ValueError("Invalid body number: " + key)
    if (
        not 0.25 <= body["model_scale"] <= 2
        or not 0 <= body["fall_damage_scale"] <= 1
        or not 0 <= body["fall_damage_max_hp_fraction"] <= 1
    ):
        raise ValueError("Body scale or fall policy exceeds SDK bounds")
    if any(body[k] != 0 for k in ["radius_cm", "height_cm", "hurt_radius_cm", "hurt_height_cm"]):
        if not (
            10 <= body["radius_cm"] <= 40
            and 2 * body["radius_cm"] <= body["height_cm"] <= 150
            and 10 <= body["hurt_radius_cm"] <= 45
            and 2 * body["hurt_radius_cm"] <= body["hurt_height_cm"] <= 160
        ):
            raise ValueError("Body dimensions exceed SDK bounds")
    profile["body"] = body
    camera = profile.get("camera", {"distance_scale": 1.0})
    scale = camera.get("distance_scale")
    if (
        set(camera) != {"distance_scale"}
        or isinstance(scale, bool)
        or not isinstance(scale, (int, float))
        or not math.isfinite(scale)
        or not 1 <= scale <= 2
    ):
        raise ValueError("Invalid camera distance scale (1..2)")
    profile["camera"] = camera
    names = []
    for attack in profile["attacks"]:
        name = attack["name"]
        if not re.fullmatch("[a-z][a-z0-9_]{0,30}", name) or name in names:
            raise ValueError("Attack names must be unique identifiers")
        names.append(name)
        if any(flag not in FLAGS for flag in attack["flags"]):
            raise ValueError("Unknown attack flag")
        knockback = attack.get("knockback_cm", 0)
        if (
            isinstance(knockback, bool)
            or not isinstance(knockback, (int, float))
            or not math.isfinite(knockback)
            or not 0 <= knockback <= 200
        ):
            raise ValueError("Attack knockback must be 0..200 cm")
    if any(cap not in CAPS for cap in profile["capabilities"]):
        raise ValueError("Unknown capability")
    runtime = profile.get("assets", {}).get("runtime")
    if runtime and not re.fullmatch("[a-zA-Z0-9_-]+\\.dll", runtime):
        raise ValueError("Runtime must be a DLL filename")
    for obj in [profile, *profile["attacks"]]:
        if any(isinstance(v, (int, float)) and not math.isfinite(v) for v in obj.values()):
            raise ValueError("Profile numbers must be finite")
    if profile.get("automatic_attack") is not None and profile["automatic_attack"] not in names:
        raise ValueError("Automatic attack does not exist")
    return folder, profile


def profile_header(profile, path):
    """Emit ABI initializers from a validated manifest; preserve SDK field order."""

    # Initializer code is emitted only from validated numeric values and JSON-escaped strings.
    def num(value):
        literal = format(float(value), ".9g")
        return literal + ("f" if "." in literal or "e" in literal else ".0f")

    def integer(value):
        if not isinstance(value, int) or isinstance(value, bool) or value < 0:
            raise ValueError("Expected non-negative integer")
        return str(value)

    attacks = []
    for attack in profile["attacks"]:
        attacks.append(
            "{"
            + ",".join(
                [
                    json.dumps(attack["name"]),
                    num(attack["multiplier"]),
                    num(attack["poise"]),
                    integer(attack["stamina_cost"]),
                    str(sum(FLAGS[flag] for flag in attack["flags"])),
                    integer(attack["active_ticks"]),
                    integer(attack["max_targets"]),
                    num(attack["range_cm"]),
                    num(attack["min_height_cm"]),
                    num(attack["max_height_cm"]),
                    num(attack.get("automatic_radius_cm", 0)),
                    num(attack.get("automatic_min_height_cm", 0)),
                    num(attack.get("automatic_max_height_cm", 0)),
                ]
            )
            + "}"
        )
    names = [attack["name"] for attack in profile["attacks"]]
    automatic_attack = (
        names.index(profile["automatic_attack"]) if profile.get("automatic_attack") else -1
    )
    body = profile.get("body", BODY_DEFAULTS)
    body_code = (
        ",".join(num(body[k]) for k in list(BODY_DEFAULTS)[:-1])
        + ","
        + str(int(body["nonlethal_falls"]))
    )
    text = (
        "static const SC_Descriptor characterProfile={sizeof(SC_Descriptor),SC_ABI_VERSION,"
        + ",".join(
            [
                json.dumps(profile["id"]),
                json.dumps(profile["display_name"]),
                integer(profile["max_triangles"]),
                integer(profile["texture"][0]),
                integer(profile["texture"][1]),
                str(sum(CAPS[capability] for capability in profile["capabilities"])),
                integer(profile.get("audio_rate", 0)),
                str(len(attacks)),
                str(automatic_attack),
                num(profile["base_power"]),
                num(profile["upgrade_power"]),
                num(profile["dexterity_weight"]),
            ]
        )
        + ",{"
        + ",".join(attacks)
        + "},{"
        + body_code
        + "}};\n"
    )
    path.write_text(text, encoding="utf-8")
    with path.open("a", encoding="utf-8") as stream:
        stream.write(
            "static const SC_CameraProfile characterCameraProfile={sizeof(SC_CameraProfile),SC_CAMERA_PROFILE_VERSION,"
            + num(profile.get("camera", {"distance_scale": 1.0})["distance_scale"])
            + "};\n"
        )
    with path.open("a", encoding="utf-8") as stream:
        stream.write(
            "static const SC_ImpactProfile characterImpactProfile={sizeof(SC_ImpactProfile),SC_IMPACT_PROFILE_VERSION,"
            + str(len(attacks))
            + ",1200,80,{"
            + ",".join(num(attack.get("knockback_cm", 0)) for attack in profile["attacks"])
            + "}};\n"
        )
