"""Event collision ownership: literal IDs, deduplication, unsafe arguments."""

from pathlib import Path
from types import SimpleNamespace as N
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "hosts/ds1ptde"))
from prepare_collision import controlled_collisions


class CollisionPolicy(unittest.TestCase):
    def command(self, entity, category=2011, index=1, replacements=()):
        return N(
            category=category,
            index=index,
            args_list=[entity, 0],
            event_arg_replacements=[N(arg_index=i) for i in replacements],
        )

    def test_only_state_commands(self):
        events = {
            1: N(
                instructions=[
                    self.command(123),
                    self.command(123),
                    self.command(456, index=2),
                    self.command(789, category=2004),
                ]
            )
        }
        self.assertEqual(controlled_collisions(events), {123, 456})

    def test_unknown_entity_rejected(self):
        with self.assertRaisesRegex(ValueError, "parameterized"):
            controlled_collisions({1: N(instructions=[self.command(0, replacements=[0])])})

    def test_parameterized_target_retains_all_candidates_natively(self):
        events = {1: N(instructions=[self.command(0, replacements=[0])])}
        self.assertEqual(controlled_collisions(events, [-1, 123, 456]), {123, 456})

    def test_parameterized_enable_keeps_literal_entity(self):
        self.assertEqual(
            controlled_collisions({1: N(instructions=[self.command(123, replacements=[1])])}), {123}
        )


if __name__ == "__main__":
    unittest.main()
