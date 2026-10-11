#pragma once
static SCFeedback::Pause feedbackPause;
static bool feedbackFocused = false;
static SCFeedback::Group feedbackGroups;
static std::vector<SCFeedback::Receipt> feedbackReceipts;

struct FeedbackFlash {
    Vec at;
    DWORD born;
    SCFeedback::Envelope envelope;
    int serial;
};

static std::vector<FeedbackFlash> feedbackFlashes;
static bool feedbackEnabled = true, feedbackStopEnabled = true, feedbackSoundEnabled = true,
            feedbackFlashEnabled = true, feedbackRumbleEnabled = true;
static float feedbackVolume = .65f, feedbackRumbleScale = .75f;
static HWAVEOUT feedbackAudio = nullptr;
static WAVEHDR feedbackHeaders[4] = {};
static int16_t feedbackPCM[4][12480] = {};

static void feedbackAudioInit() {
    static bool attempted = false;
    if (attempted)
        return;
    attempted = true;
    WAVEFORMATEX fmt = {};
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 48000;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = 4;
    fmt.nAvgBytesPerSec = 192000;
    MMRESULT result = waveOutOpen(&feedbackAudio, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
    if (result == MMSYSERR_NOERROR)
        for (int i = 0; i < 4; i++) {
            auto &h = feedbackHeaders[i];
            h.lpData = (LPSTR)feedbackPCM[i];
            h.dwBufferLength = sizeof(feedbackPCM[i]);
            waveOutPrepareHeader(feedbackAudio, &h, sizeof(h));
        }
    logf("impact audio open result=%u", result);
}

static void feedbackSound(const SCFeedback::Envelope &e, int serial) {
    if (!feedbackAudio)
        return;
    for (int i = 0; i < 4; i++)
        if (!(feedbackHeaders[i].dwFlags & WHDR_INQUEUE)) {
            SCFeedback::sound(feedbackPCM[i], 6240, 48000, e.strength, uint32_t(serial),
                              feedbackVolume);
            auto result = waveOutWrite(feedbackAudio, &feedbackHeaders[i], sizeof(WAVEHDR));
            logf("Impact sound serial=%d result=%u", serial, result);
            return;
        }
}

static void feedbackReset() {
    feedbackPause.clear();
    feedbackGroups = {};
    feedbackReceipts.clear();
    feedbackFlashes.clear();
    feedbackRumbleStop();
    if (feedbackAudio)
        waveOutReset(feedbackAudio);
}

static void feedbackSettings() {
    feedbackEnabled = GetPrivateProfileIntA("Feedback", "Enabled", 1, ini) != 0;
    feedbackStopEnabled = GetPrivateProfileIntA("Feedback", "HitStop", 1, ini) != 0;
    feedbackSoundEnabled = GetPrivateProfileIntA("Feedback", "Sound", 1, ini) != 0;
    feedbackFlashEnabled = GetPrivateProfileIntA("Feedback", "Flash", 1, ini) != 0;
    feedbackRumbleEnabled = GetPrivateProfileIntA("Feedback", "Rumble", 1, ini) != 0;
    feedbackVolume =
        std::clamp<int>(GetPrivateProfileIntA("Feedback", "Volume", 65, ini), 0, 100) * .01f;
    feedbackRumbleScale =
        std::clamp<int>(GetPrivateProfileIntA("Feedback", "RumbleStrength", 75, ini), 0, 100) *
        .01f;
    if (feedbackEnabled && feedbackSoundEnabled)
        feedbackAudioInit();
    if (!feedbackEnabled)
        feedbackReset();
    if (!feedbackStopEnabled)
        feedbackPause.clear();
    if (!feedbackRumbleEnabled)
        feedbackRumbleStop();
    if (!feedbackFlashEnabled)
        feedbackFlashes.clear();
}

static void feedbackTrack(uintptr_t target, int id, int before, int serial, int move, Vec at,
                          bool object = false) {
    if (!feedbackEnabled || !feedbackFocused || before <= 0 || feedbackReceipts.size() >= 64)
        return;
    for (const auto &r : feedbackReceipts)
        if (r.target == target && r.object == object && r.serial == serial)
            return;
    float multiplier =
        move >= 0 && move < int(profile->attackCount) ? profile->attacks[move].multiplier : 1.f;
    feedbackReceipts.push_back(
        {target, id, serial, move, before, GetTickCount(), object, at.x, at.y, at.z, multiplier});
}

static void feedbackConfirmed(const SCFeedback::Receipt &r, int after, DWORD now) {
    debugContact({r.x, r.y, r.z}, true);
    auto e = SCFeedback::envelope(r.multiplier);
    bool first = feedbackGroups.first(r.serial);
    if (first) {
        if (feedbackStopEnabled)
            feedbackPause.start(now, e.stopMs);
        if (feedbackSoundEnabled)
            feedbackSound(e, r.serial);
        if (feedbackRumbleEnabled)
            feedbackRumble(HostController::owner(), e, feedbackRumbleScale);
    }
    if (feedbackFlashEnabled) {
        if (feedbackFlashes.size() >= 8)
            feedbackFlashes.erase(feedbackFlashes.begin());
        feedbackFlashes.push_back({{r.x, r.y, r.z}, now, e, r.serial});
    }
    char fields[360];
    snprintf(
        fields, sizeof(fields),
        ",\"serial\":%d,\"move\":\"%s\",\"target\":%d,\"object\":%s,\"hp_before\":%d,\"hp_after\":%d,\"latency_ms\":%lu,\"group_first\":%s,\"stop_ms\":%u",
        r.serial, attackName(r.move), r.id, r.object ? "true" : "false", r.before, after,
        now - r.began, first ? "true" : "false", first && feedbackStopEnabled ? e.stopMs : 0);
    harnessEvent("impact_confirmed", fields);
    logf("Impact confirmed%s", fields);
    diagnosticEvent(SCDiagnostics::Kind::HitConfirmed, r.before, after);
}

static void feedbackPoll() {
    if (!feedbackEnabled)
        return;
    DWORD now = GetTickCount();
    for (size_t i = 0; i < feedbackReceipts.size();) {
        const auto r = feedbackReceipts[i];
        int hp = 0, id = 0;
        bool valid = false;
        if (r.object) {
            int16_t shortHP = 0;
            valid = get(r.target + 4, id) && get(r.target + 0x2a, shortHP);
            hp = shortHP;
        } else
            valid = get(r.target + 4, id) && nativeTarget(r.target, uint32_t(id)) &&
                    get(r.target + 0x2d4, hp);
        int result = r.sample(id, hp, valid, now);
        if (result == 1)
            feedbackConfirmed(r, hp, now);
        if (result)
            feedbackReceipts.erase(feedbackReceipts.begin() + i);
        else
            ++i;
    }
    feedbackFlashes.erase(std::remove_if(feedbackFlashes.begin(), feedbackFlashes.end(),
                                         [now](const FeedbackFlash &f) {
                                             return DWORD(now - f.born) >= f.envelope.flashMs;
                                         }),
                          feedbackFlashes.end());
}

static void feedbackDraw(IDirect3DDevice9 *d) {
    if (feedbackFlashes.empty())
        return;

    struct V {
        float x, y, z;
        DWORD color;
    };

    std::vector<V> vertices;
    Vec forward = unit(cameraForward()), right = unit(cross({0, 1, 0}, forward)),
        up = cross(forward, right);
    DWORD now = GetTickCount();
    for (const auto &f : feedbackFlashes) {
        float t = float(DWORD(now - f.born)) / f.envelope.flashMs;
        if (t >= 1)
            continue;
        float radius = (.38f + .18f * f.envelope.strength) * (1 + 1.7f * t);
        DWORD alpha = DWORD(255 * (1 - t));
        Vec center = f.at - forward * .25f;
        auto vertex = [&](Vec p, DWORD color) { vertices.push_back({p.x, p.y, p.z, color}); };
        for (int i = 0; i < 8; i++) {
            float a = i * .78539816f + f.serial * .31f;
            Vec dir = right * cosf(a) + up * sinf(a), side = right * (-sinf(a)) + up * cosf(a);
            vertex(center + side * (radius * .15f), alpha << 24 | 0xfff5cb);
            vertex(center + dir * (radius * (i % 2 ? .6f : 1.f)), (alpha / 3) << 24 | 0xffb34f);
            vertex(center - side * (radius * .15f), alpha << 24 | 0xfff5cb);
        }
    }
    if (vertices.empty())
        return;
    d->SetTexture(0, nullptr);
    d->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    d->SetRenderState(D3DRS_ZENABLE, TRUE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    // Keep correct alpha in the intermediate character layer; later composite is
    // below native UI. Depth tests retain wall/enemy occlusion.
    d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    d->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    d->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    d->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
    auto result =
        d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, vertices.size() / 3, vertices.data(), sizeof(V));
    static bool logged = false;
    if (!logged) {
        logf("Impact draw result=%08lx", result);
        logged = true;
    }
}
