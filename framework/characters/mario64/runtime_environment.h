/* Adapter-owned entry point beside the pinned libsm64 implementation. */
SM64_LIB_FN uint32_t sm64_mario_host_burning(int32_t id, uint32_t airborne) {
    if (id < 0 || id >= s_mario_instance_pool.size || !s_mario_instance_pool.objects[id])
        return 0;
    struct MarioInstance *instance = s_mario_instance_pool.objects[id];
    global_state_bind(instance->globalState);
    instance->hostHealth = true;
    gMarioState->marioObj->oMarioBurnTimer = 0;
    gMarioState->health = 0x880;
    gMarioState->hurtCounter = 0;
    gMarioState->invincTimer = 0;
    set_mario_action(gMarioState, airborne ? ACT_BURNING_FALL : ACT_BURNING_GROUND, 0);
    play_sound(SOUND_MARIO_ON_FIRE, gMarioState->marioObj->header.gfx.cameraToObject);
    return 1;
}
