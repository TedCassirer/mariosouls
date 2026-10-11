#pragma once
#include <shellapi.h>
#include <shlobj.h>
#include <new>
#include "../../core/diagnostics.h"
#include "report_files.h"
#include "build.generated.h"
static SCDiagnostics::Journal diagnosticJournal;
static volatile LONG diagnosticWorkerBusy = 0;

struct DiagnosticJob {
    std::string json, review;
};

static DWORD WINAPI diagnosticWorker(void *raw) {
    auto *job = static_cast<DiagnosticJob *>(raw);
    wchar_t local[MAX_PATH]{};
    bool ok = false;
    DWORD error = 0;
    if (SUCCEEDED(
            SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, local))) {
        std::wstring base = std::wstring(local) + L"\\MarioSouls";
        bool made =
            CreateDirectoryW(base.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
        base += L"\\Reports";
        made = made &&
               (CreateDirectoryW(base.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS);
        if (made) {
            auto files = SCReportFiles::save(base, job->json, job->review);
            ok = files.ok;
            error = files.error;
            if (ok) {
                auto opened = ShellExecuteW(nullptr, L"open", files.review.c_str(), nullptr,
                                            nullptr, SW_SHOWNORMAL);
                if (INT_PTR(opened) <= 32) {
                    std::wstring message =
                        L"Your report was saved, but the browser could not open. Open review.html in:\n" +
                        files.directory;
                    MessageBoxW(nullptr, message.c_str(), L"MarioSouls bug report",
                                MB_OK | MB_ICONINFORMATION);
                }
            }
        } else
            error = GetLastError();
    }
    if (!ok) {
        wchar_t message[200];
        swprintf(
            message, 200,
            L"Could not save the bug report (Windows error %lu). Check free disk space and access to Local AppData. No issue was submitted.",
            error);
        MessageBoxW(nullptr, message, L"MarioSouls bug report", MB_OK | MB_ICONWARNING);
    }
    delete job;
    InterlockedExchange(&diagnosticWorkerBusy, 0);
    return 0;
}

static void diagnosticEvent(SCDiagnostics::Kind kind, int before, int after) {
    if (diagnosticEnabled)
        diagnosticJournal.event(kind, GetTickCount(), before, after);
}

static void diagnosticsTick() {
    DWORD now = GetTickCount(), pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    bool focus = pid == GetCurrentProcessId();
    static SCDiagnostics::Trigger trigger;
    bool requested = trigger.update((GetAsyncKeyState(VK_F11) & 0x8000) != 0, focus, now);
    static DWORD configured = 0, sampled = 0;
    if (!configured || DWORD(now - configured) >= 2000) {
        configured = now;
        bool enabled = GetPrivateProfileIntA("Diagnostics", "Enabled", 1, ini) != 0;
        if (!enabled && diagnosticEnabled)
            diagnosticJournal = {};
        diagnosticEnabled = enabled;
    }
    if (!diagnosticEnabled)
        return;
    if (!requested && DWORD(now - sampled) < 50)
        return;
    sampled = now;
    SCDiagnostics::Frame f{};
    f.time = now;
    f.generation = characterSession.generation();
    f.map = currentMap();
    f.cachedMap = loadedMap;
    f.owner = int(characterSession.owner());
    // Reject a cached actor replaced during loading. Read-only reporting is allowed
    // on any save; the guarded harness remains the only route for automated actions.
    f.actor = player && posData && chain(playerSlot, {0, 4, 0}) == player &&
              chain(player + 0x28, {0, 0x1c}) == posData;
    f.requested = characterSession.requested();
    f.presented = characterPresented();
    f.rig = character >= 0 && guest && profile;
    f.focus = focus;
    f.controller = pollController(now).connected;
    if (f.actor) {
        auto value = [&](unsigned offset) {
            int result = -1;
            return get(player + offset, result) ? result : -1;
        };
        f.hp = value(0x2d4);
        f.maxHP = value(0x2d8);
        f.stamina = value(0x2e4);
        f.maxStamina = value(0x2e8);
        Vec position{};
        f.positionValid = get(posData + 0x10, position);
        f.position[0] = position.x;
        f.position[1] = position.y;
        f.position[2] = position.z;
    }
    f.guestValid = f.rig && characterOwned();
    if (f.guestValid) {
        for (int i = 0; i < 3; i++)
            f.guestPosition[i] = state.position[i] * .01f;
        f.guestPosition[0] += origin.x;
        f.guestPosition[1] += origin.y;
        f.guestPosition[2] += origin.z;
        f.floor = state.floorHeight * .01f + origin.y;
        f.speed = state.forwardSpeed;
        f.action = state.rawAction;
        f.animation = state.rawAnimation;
        f.attack = state.attackId;
        f.flags = state.flags;
    }
    f.attackSerial = characterWindow.serial;
    f.attackExhausted = characterWindow.exhausted;
    f.combat = combatEnabled;
    f.combatReady = nativeDamage && characterDamageReady;
    f.right = equipmentCurrent.right.id;
    f.left = equipmentCurrent.left.id;
    f.contacts = int(worldContacts.size());
    f.triangles = int(mapVertices.size() / 3);
    f.width = diagnosticSceneWidth;
    f.height = diagnosticSceneHeight;
    AcquireSRWLockShared(&worldLock);
    f.worldReady = worldMode == 1 && worldMotionReady && worldBinding.valid();
    f.supported = worldNativeSupport.supported();
    f.submissions = worldSubmissions;
    f.acknowledgements = worldAcknowledgements;
    f.rejected = worldRejected;
    ReleaseSRWLockShared(&worldLock);
    // Native action ownership comes from the active route; reporting never queries
    // guest floors, invokes an animation, or changes gameplay/control ownership.
    f.nativeAction = nativeActionOwned();
    diagnosticJournal.observe(f);
    if (!requested || InterlockedCompareExchange(&diagnosticWorkerBusy, 1, 0) != 0)
        return;
    auto *job = new (std::nothrow) DiagnosticJob;
    if (!job) {
        InterlockedExchange(&diagnosticWorkerBusy, 0);
        return;
    }
    std::string id = profile ? profile->id : "unknown";
    job->json = diagnosticJournal.json(id, SC_BUILD_VERSION, SC_BUILD_COMMIT, SC_BUILD_SOURCE,
                                       SC_BUILD_DIRTY);
    job->review = SCDiagnostics::reviewHTML(diagnosticJournal.body(
        id, SC_BUILD_VERSION, SC_BUILD_COMMIT, SC_BUILD_SOURCE, SC_BUILD_DIRTY));
    HANDLE worker = CreateThread(nullptr, 0, diagnosticWorker, job, 0, nullptr);
    if (worker)
        CloseHandle(worker);
    else {
        delete job;
        InterlockedExchange(&diagnosticWorkerBusy, 0);
        logf("Bug report worker unavailable; no issue submitted");
    }
}
