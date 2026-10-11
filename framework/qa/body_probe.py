"""Read-only native body inspection for the dedicated Asylum test saves."""

import json
import struct
import sys
from host import Host


def inspect(h):
    s = h.snapshot(require_test=False)
    assert s["player"]["name"] in ("Mario QA", "test") and 1 <= s["stats"]["level"] <= 10
    assert s["map"] == 1801
    actor = int(s["player"]["reference"], 16)
    physics = h.chain(actor, 0x28, 0x1C)
    proxy = h.u(physics + 0x38)
    phantom = h.u(proxy + 0x60)

    def dump(addr, n):
        data = h.read(addr, n)
        return [
            {
                "off": hex(i),
                "u": hex(struct.unpack_from("<I", data, i)[0]),
                "f": round(struct.unpack_from("<f", data, i)[0], 6),
            }
            for i in range(0, n, 4)
        ]

    return dict(
        player=s["player"],
        stats=s["stats"],
        physics=hex(physics),
        proxy=hex(proxy),
        phantom=hex(phantom),
        physics_data=dump(physics, 0x270),
        proxy_data=dump(proxy, 0xB0),
        phantom_data=dump(phantom, 0xC0),
        shapes=[
            {"address": hex(h.u(physics + off)), "data": dump(h.u(physics + off), 0x40)}
            for off in (0x3C, 0x40)
        ],
        actor_data=dump(actor, 0x90),
    )


if __name__ == "__main__":
    h = Host(int(sys.argv[1]))
    try:
        print(json.dumps(inspect(h), indent=2))
    finally:
        h.close()
