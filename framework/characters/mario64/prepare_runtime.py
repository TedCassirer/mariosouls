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
    def staged(name,replacements):
        path=repo/'src'/name
        source=path.read_text()
        for before,after,count in replacements:
            if source.count(before)!=count:raise RuntimeError('Pinned runtime equipment anchor changed: '+name+' '+before)
            source=source.replace(before,after)
        path.write_text(source)
    (repo/'src/sc_equipment.h').write_bytes(Path(__file__).with_name('runtime_equipment.h').read_bytes())
    staged('libsm64.c',[
        ('s16 lastPose[63],blendFrom[63];u16 poseIndices[126];',
         's16 lastPose[63],blendFrom[63];u16 poseIndices[126];\n    uint32_t equipmentRight,equipmentLeft,equipmentSwing,equipmentValid;\n    float equipmentPhase,equipmentGuard,equipmentRightHand[16],equipmentLeftHand[16];',1),
        ('#include "host_pose.h"','#include "sc_equipment.h"\n#include "host_pose.h"',1),
        ('    update_button( inputs->buttonA, A_BUTTON );',
         '    sc_equipment_before_tick((struct MarioInstance *)s_mario_instance_pool.objects[marioId]);\n    update_button( inputs->buttonA, A_BUTTON );',1),
    ])
    staged('host_pose.h',[
        ('struct AnimInfo *info=&gMarioState->marioObj->header.gfx.animInfo;',
         'sc_equipment_begin(instance);\n    struct AnimInfo *info=&gMarioState->marioObj->header.gfx.animInfo;',1),
        ('geo_process_root_hack_single_node(s_mario_graph_node);',
         'geo_process_root_hack_single_node(s_mario_graph_node);sc_equipment_end();',2),
    ])
    staged('decomp/game/rendering_graph_node.c',[
        ('float sm64_host_right_hand[4][4];',
         'extern void sc_mario_equipment_rotation(int attributeOffset,s16 *rotation);\nextern void sc_mario_equipment_hand(int attributeOffset,const float *matrix);\nfloat sm64_host_right_hand[4][4];',1),
        ('    mtxf_rotate_xyz_and_translate(matrix, translation, rotation);',
         '    if(hostAnimAttributes)sc_mario_equipment_rotation((int)(gCurrAnimAttribute-hostAnimAttributes),rotation);\n    mtxf_rotate_xyz_and_translate(matrix, translation, rotation);',1),
        ('    if(hostAnimAttributes && gCurrAnimAttribute-hostAnimAttributes==78)memcpy(sm64_host_right_hand,gMatStack[gMatStackIndex],sizeof(sm64_host_right_hand));',
         '    if(hostAnimAttributes)sc_mario_equipment_hand((int)(gCurrAnimAttribute-hostAnimAttributes),&gMatStack[gMatStackIndex][0][0]);\n    if(hostAnimAttributes && gCurrAnimAttribute-hostAnimAttributes==78)memcpy(sm64_host_right_hand,gMatStack[gMatStackIndex],sizeof(sm64_host_right_hand));',1),
    ])
