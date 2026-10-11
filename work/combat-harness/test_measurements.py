import unittest
from runner import summarize,classify
class Measurements(unittest.TestCase):
 def test_native_return_is_not_damage(self):
  r=summarize([dict(type='hit_attempt'),dict(type='native_call_return',native_return=1)])
  self.assertEqual(r['confirmed_hits'],0);self.assertEqual(r['damage_to_target'],0)
 def test_delayed_health_loss(self):
  r=summarize([dict(type='native_call_return',native_return=0),dict(type='confirmed_target_hp_loss',before=44,after=30,damage=14)])
  self.assertEqual(r['confirmed_hits'],1);self.assertEqual(r['damage_to_target'],14)
 def test_heal_does_not_count_as_damage(self):
  r=summarize([dict(type='player_heal',before=200,after=594,damage=-394)])
  self.assertEqual(r['damage_to_player'],0)
 def test_native_stamina_recovery(self):
  r=summarize([dict(type='observation',stamina=v) for v in [95,80,85,95,95]])
  self.assertEqual(r['stamina_decreases'],15)
 def test_unidentified_combo_is_inconclusive(self):
  r=summarize([dict(type='input_frame',animation=0x67,action=0x800380)])
  self.assertEqual(classify('combo',r,[])[0],'inconclusive')
 def test_missed_attack_is_not_balance_evidence(self):
  r=summarize([dict(type='input_frame',animation=0x3d,action=0x008008a9)])
  self.assertEqual(classify('ground_pound',r,[])[0],'inconclusive')
 def test_negative_control_damage_fails(self):
  r=summarize([dict(type='confirmed_target_hp_loss',damage=14)])
  self.assertEqual(classify('punch_out',r,[])[0],'failed')
 def test_one_hit_does_not_accept_three(self):
  r=summarize([dict(type='confirmed_player_hp_loss',damage=32)]*3)
  self.assertEqual(classify('incoming_hit',r,[])[0],'inconclusive')
 def test_kill_reset_requires_kill(self):
  r=summarize([]);r['killed']=False
  self.assertEqual(classify('kill_reset',r,[])[0],'inconclusive')
 def test_dive_damage_must_belong_to_dive_attempt(self):
  r=summarize([dict(type='input_frame',action=0x0188088a,animation=0x88),dict(type='native_call_return',action=0x018008ac,attempt_id=1),dict(type='confirmed_target_hp_loss',attempt_id=1,damage=14)])
  self.assertEqual(classify('dive',r,[])[0],'inconclusive')
if __name__=='__main__':unittest.main()
