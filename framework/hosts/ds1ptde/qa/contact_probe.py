"""Read-only contact/target geometry evidence, guarded to Mario QA in the Asylum."""

import json
import struct
import argparse


def geometry(h, actor):
    physics = h.chain(actor, 0x28, 0x1C)
    shapes = []
    for offset in (0x3C, 0x40):
        shape = h.u(physics + offset)
        if not shape:
            shapes.append(None)
            continue
        shapes.append(
            dict(
                address=hex(shape),
                vtable=hex(h.u(shape)),
                kind=h.u(shape + 0xC),
                radius=h.f(shape + 0x10),
                points=list(struct.unpack("<8f", h.read(shape + 0x20, 32))),
            )
        )
    return dict(
        physics=hex(physics),
        position=h.v(physics + 0x10),
        radius=h.f(physics + 0x208),
        height=h.f(physics + 0x204),
        shapes=shapes,
    )


def observe(h, target=1810800):
    from host import StateError

    s = h.snapshot(target)
    actor = int(s["player"]["reference"], 16)
    physics = h.chain(actor, 0x28, 0x1C)
    proxy = h.u(physics + 0x38)
    data = h.u(proxy + 0x10)
    count = h.i(proxy + 0x14)
    self_body = h.u(proxy + 0x60) + 0x10
    contacts = []
    for i in range(max(0, min(count, 64))):
        at = data + i * 0x30
        a, b = h.u(at + 0x20), h.u(at + 0x28)
        body = b if a == self_body else a if b == self_body else 0
        c = dict(point=h.v(at), normal=h.v(at + 0x10), distance=h.f(at + 0x1C))
        try:
            obj = body + struct.unpack("<b", h.read(body + 0x10, 1))[0]
            tag = h.u(h.u(obj + 0xC))
            c.update(kind=struct.unpack("<H", h.read(tag + 4, 2))[0], owner=hex(h.u(tag + 8)))
        except StateError:
            pass
        contacts.append(c)
    s["contacts"] = contacts
    s["player_geometry"] = geometry(h, actor)
    if s["target"]:
        s["target_geometry"] = geometry(h, int(s["target"]["reference"], 16))
    return s


def main(arguments=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pid", type=int)
    parser.add_argument("target", nargs="?", type=int, default=1810800)
    args = parser.parse_args(arguments)
    from host import Host

    host = Host(args.pid)
    try:
        print(json.dumps(observe(host, args.target), indent=2))
    finally:
        host.close()


if __name__ == "__main__":
    main()
