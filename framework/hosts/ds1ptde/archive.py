"""Read the user's PTDE BHD5 archives; never modify game data."""

from pathlib import Path
import struct


class Archives:
    def __init__(self, game):
        self.game = Path(game)
        self.entries = {}
        for i in range(4):
            data = (self.game / f"dvdbnd{i}.bhd5").read_bytes()
            if data[:5] != b"BHD5\xff":
                raise ValueError("Unexpected PTDE archive")
            count, offset = struct.unpack_from("<ii", data, 16)
            if not 0 < count < 100000:
                raise ValueError("Invalid archive buckets")
            size = (self.game / f"dvdbnd{i}.bdt").stat().st_size
            for bucket in range(count):
                n, ptr = struct.unpack_from("<ii", data, offset + bucket * 8)
                if not 0 <= n < 100000:
                    raise ValueError("Invalid archive entries")
                for j in range(n):
                    hashed, length, pos = struct.unpack_from("<IIQ", data, ptr + j * 16)
                    if pos + length > size:
                        raise ValueError("Archive entry exceeds data file")
                    self.entries[hashed] = (i, length, pos)

    @staticmethod
    def hash(path):
        h = 0
        for ch in "/" + path.replace("\\", "/").lower().lstrip("/"):
            h = (h * 37 + ord(ch)) & 0xFFFFFFFF
        return h

    def read(self, path):
        i, length, pos = self.entries[self.hash(path)]
        with (self.game / f"dvdbnd{i}.bdt").open("rb") as stream:
            stream.seek(pos)
            data = stream.read(length)
        if len(data) != length:
            raise ValueError("Incomplete archive read")
        return data
