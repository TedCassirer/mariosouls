"""Read-only evidence for the dedicated newly-created Mario Class QA fixture.
Does not broaden Host.snapshot guards or grant harness/inventory access.
"""
import argparse,json
from pathlib import Path
from host import Host,StateError

def observe(host):
    state=host.snapshot(require_test=False)
    stats=host.chain(0x1378700,0,8)
    slot=host.read(stats+0xc6,1)[0]
    attrs=state["stats"]
    if state["player"]["name"] not in ("Mario Class QA","Mario ClassQA") or state["map"]!=1801:
        raise StateError("Requires the dedicated Mario Class QA in the Asylum")
    expected=dict(level=1,vitality=10,attunement=12,endurance=11,strength=12,
                  dexterity=9,resistance=12,intelligence=10,faith=8)
    if slot!=7 or any(attrs[key]!=value for key,value in expected.items()):
        raise StateError("Starting class/stats do not match the unmodified firecaster fixture")
    state["native_class"]=slot
    state["starting_equipment_matches"]=all(state["equipment"][key]==value for key,value in
        dict(right1=1330000,right2=900000,left1=1406000,left2=900000).items())
    return state

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pid",type=int)
    parser.add_argument("--output",required=True)
    args=parser.parse_args()
    host=Host(args.pid)
    try:
        state=observe(host)
        Path(args.output).write_text(json.dumps(state,indent=2),encoding="utf-8")
        print(json.dumps(dict(name=state["player"]["name"],native_class=state["native_class"],
                             stats=state["stats"],equipment=state["equipment"],
                             starting_equipment_matches=state["starting_equipment_matches"])))
    finally:
        host.close()

if __name__=="__main__":
    main()
