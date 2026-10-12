#undef NDEBUG
#include "../core/body.h"
#include "../core/camera.h"
#include "../core/combat.h"
#include "../core/contact.h"
#include "../core/context_action.h"
#include "../core/debug_geometry.h"
#include "../core/diagnostics.h"
#include "../core/equipment.h"
#include "../core/feedback.h"
#include "../core/hazard.h"
#include "../core/host_capabilities.h"
#include "../core/impact.h"
#include "../core/kinematic.h"
#include "../core/launcher.h"
#include "../core/mesh_pack.h"
#include "../core/motion.h"
#include "../core/native_action.h"
#include "../core/projectile.h"
#include "../core/qa_identity.h"
#include "../core/session.h"
#include "../core/starting_class.h"
#include "../core/timing.h"
#include "../core/validation.h"
#include <cassert>
#include <cstdio>

int main() {
    SC_Body body{.8f, 15, 100, 15, 100, .1f, .2f, 1};
    assert(SCBody::valid(body));
    assert(SCBody::landingDamage(1000, 100, 100, body) == 20);
    assert(SCBody::landingDamage(1000, 1, 100, body) == 0);
    assert(SCBody::modelCoordinate(100, 0, .8f) == 80);
    SCDiagnostics::Journal journal;
    auto json = journal.json("probe", "version", "commit", "source", false, "simulated");
    assert(json.find("\"host\":\"simulated\"") != std::string::npos);
    assert(json.find("ds1ptde") == std::string::npos);
    SCNativeAction::Buttons buttons;
    assert(!buttons.sample(true, false, true, 1, 0).attack);
    assert(!buttons.sample(false, false, true, 1, 0).attack);
    assert(buttons.sample(true, false, true, 1, 0).attack);
    assert(!buttons.sample(true, false, true, 2, 0).attack);
    puts(
        "PASS all shared headers, body policy, host-neutral diagnostic identity and native-action edges");
}
