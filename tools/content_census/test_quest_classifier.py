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

    def test_simultaneous_prev_and_next_quest_semantics(self):
        # Regression test for quest with both PrevQuestID and NextQuestID (e.g. Quest 2)
        # Authored data:
        # Quest 2: Level 30, MinLevel 20
        # NextQuest 247: Level 30, MinLevel 20
        # PrevQuest 6383: Level 20, MinLevel 20
        # Under UNION ALL, two distinct rows are produced with distinct LinkTypes:
        row_next = (2, 30, 20, 247, 30, 20, "NextQuest")
        row_prev = (2, 30, 20, 6383, 20, 20, "PrevQuest")

        # 1. Verify distinct link types
        self.assertNotEqual(row_next[6], row_prev[6])
        self.assertEqual(row_next[6], "NextQuest")
        self.assertEqual(row_prev[6], "PrevQuest")

        # 2. Verify NextQuest direction (2 -> 247, Level 30 -> 30) evaluates to PASS
        status_next, reason_next = classify_chain_link(
            row_next[0], row_next[1], row_next[2],
            row_next[3], row_next[4], row_next[5],
            row_next[6], self.mappings, max_cap=60
        )
        self.assertEqual(status_next, "PASS")
        self.assertIn("monotonic and reachable", reason_next)

        # 3. Verify PrevQuest direction (6383 -> 2, Level 20 -> 30) evaluates to PASS
        # (PrevQuest reverses Q1/Q2 so predecessor is 6383 and successor is 2)
        status_prev, reason_prev = classify_chain_link(
            row_prev[0], row_prev[1], row_prev[2],
            row_prev[3], row_prev[4], row_prev[5],
            row_prev[6], self.mappings, max_cap=60
        )
        self.assertEqual(status_prev, "PASS")
        self.assertIn("monotonic and reachable", reason_prev)

        # 4. Verify that mislabeling PrevQuest as NextQuest would have produced an incorrect regression warning
        status_mislabelled, reason_mislabelled = classify_chain_link(
            row_prev[0], row_prev[1], row_prev[2],
            row_prev[3], row_prev[4], row_prev[5],
            "NextQuest", self.mappings, max_cap=60
        )
        self.assertEqual(status_mislabelled, "WARNING")
        self.assertIn("regression gap", reason_mislabelled)

if __name__ == "__main__":
    unittest.main()
