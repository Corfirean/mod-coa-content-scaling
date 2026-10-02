#!/usr/bin/env python3
"""
Unit tests for quest progression chain classification rules.
Validates PASS, WARNING, and CONFLICT states across all classifier invariants.
"""

import unittest
from generate_progression_reports import classify_chain_link

class TestQuestChainClassifier(unittest.TestCase):
    def setUp(self):
        # Sample level mapping corresponding to Cap 60 (All Eras)
        self.mappings = {
            "Classic": {
                "1": 1, "4": 3, "5": 4, "10": 8, "20": 15, "24": 18,
                "30": 23, "42": 32, "47": 35, "48": 36, "60": 45
            },
            "TBC": {
                "58": 45, "60": 47, "62": 48, "65": 51, "70": 55
            },
            "WotLK": {
                "68": 55, "70": 56, "75": 58, "77": 59, "80": 60
            }
        }

    def test_reachable_same_era_passes(self):
        # Monotonic quest in same era
        status, reason = classify_chain_link(
            10, 10, 8, 20, 20, 15, "NextQuest", self.mappings, max_cap=60
        )
        self.assertEqual(status, "PASS")
        self.assertIn("monotonic and reachable", reason)

    def test_large_backwards_breadcrumb_is_warning(self):
        # Breadcrumb pointing to lower level quest
        status, reason = classify_chain_link(
            30, 30, 25, 10, 10, 5, "Breadcrumb", self.mappings, max_cap=60
        )
        self.assertEqual(status, "WARNING")
        self.assertIn("Breadcrumb level regression", reason)

    def test_dynamic_quest_level_is_warning(self):
        # Quest with dynamic QuestLevel (-1)
        status, reason = classify_chain_link(
            100, -1, 10, 101, 20, 15, "NextQuest", self.mappings, max_cap=60
        )
        self.assertEqual(status, "WARNING")
        self.assertIn("Dynamic/special", reason)

    def test_target_min_level_above_cap_is_conflict(self):
        # Target MinLevel exceeds cap in layout (e.g. MinLevel 75 mapped to eff 58, but cap is 50)
        status, reason = classify_chain_link(
            100, 50, 45, 101, 75, 75, "NextQuest", self.mappings, max_cap=50
        )
        self.assertEqual(status, "CONFLICT")
        self.assertIn("exceeds level cap", reason)

    def test_special_internal_min_level_is_warning(self):
        # Target with internal marker (e.g. MinLevel 200/255)
        status, reason = classify_chain_link(
            100, 60, 45, 101, 60, 200, "NextQuest", self.mappings, max_cap=60
        )
        self.assertEqual(status, "WARNING")
        self.assertIn("Special/internal MinLevel flag", reason)

    def test_target_min_level_unreachable_after_source_is_conflict(self):
        # Source at level 24 (eff 18), next quest requires min 60 (eff 45) -> gap 27 levels
        status, reason = classify_chain_link(
            6681, 24, 24, 6701, 60, 60, "NextQuest", self.mappings, max_cap=60
        )
        self.assertEqual(status, "CONFLICT")
        self.assertIn("unreachable after predecessor", reason)

    def test_target_min_greater_than_quest_level_is_conflict(self):
        # Static quest where MinLevel (5) > QuestLevel (4)
        status, reason = classify_chain_link(
            5622, 4, 5, 5621, 4, 5, "PrevQuest", self.mappings, max_cap=60
        )
        self.assertEqual(status, "CONFLICT")
        self.assertIn("exceeds quest level", reason)

    def test_valid_cross_era_transition_passes(self):
        # Classic end (lvl 60 -> eff 45) to TBC entry (lvl 58 -> eff 45, min 58)
        status, reason = classify_chain_link(
            9000, 60, 55, 9001, 58, 58, "NextQuest", self.mappings, max_cap=60
        )
        self.assertEqual(status, "PASS")

    def test_disabled_target_era_is_conflict(self):
        # TBC disabled in active layout
        status, reason = classify_chain_link(
            9000, 60, 55, 9001, 62, 58, "NextQuest", self.mappings,
            active_eras={"Classic"}, max_cap=60
        )
        self.assertEqual(status, "CONFLICT")
        self.assertIn("disabled in active layout", reason)

if __name__ == "__main__":
    unittest.main()
