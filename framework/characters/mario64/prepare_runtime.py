"""Apply narrow body-query edits to a generated copy of the pinned dependency.

Original dependency/checkpoints remain untouched. No extracted source is tracked.
"""
from pathlib import Path

def prepare(repo):
    def edit(name,replacements):
        path=repo/'src/decomp/game'/name
        source=path.read_text()
        source='extern float scMarioWallRadius, scMarioBodyHeight;\n'+source
        for before,after,count in replacements:
            if source.count(before)!=count:raise RuntimeError('Pinned runtime body anchor changed: '+name+' '+before)
            source=source.replace(before,after)
        path.write_text(source)
    edit('mario.c',[
        ('f32 offset, f32 radius) {','f32 offset, f32 radius) {\n    offset *= scMarioBodyHeight / 160.0f;\n    radius *= scMarioWallRadius / 50.0f;',1),
        ('&m->pos[2], 60.0f, 50.0f);','&m->pos[2], 60.0f * scMarioBodyHeight / 160.0f, scMarioWallRadius);',1),
        ('ceilToFloorDist <= 150.0f','ceilToFloorDist <= scMarioBodyHeight - 10.0f',1),
        ('m->marioObj->hitboxHeight = 160.0f;','m->marioObj->hitboxHeight = scMarioBodyHeight;',1),
        ('m->marioObj->hitboxHeight = 100.0f;','m->marioObj->hitboxHeight = scMarioBodyHeight * .625f;',1),
    ])
    edit('mario_step.c',[
        ('nextPos[1] + 160.0f >= ceilHeight','nextPos[1] + scMarioBodyHeight >= ceilHeight',1),
        ('floorHeight + 160.0f >= ceilHeight','floorHeight + scMarioBodyHeight >= ceilHeight',1),
        ('ceilHeight - floorHeight > 160.0f','ceilHeight - floorHeight > scMarioBodyHeight',1),
        ('nextPos[1] + 160.0f > ceilHeight','nextPos[1] + scMarioBodyHeight > ceilHeight',1),
    ])
