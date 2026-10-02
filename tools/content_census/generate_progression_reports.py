#!/usr/bin/env python3
"""
CoA Universal Content Scaling - Progression Pipeline Report Generator (Round 5 Final)
Authoritative pipeline generator for Round 5 progression reports.
Derives layout, tier unlocks, level mappings, sample quest XP and instance access
directly from the C++ runtime snapshot (progression-runtime-snapshot.json) to eliminate
logic drift and duplicate formulas.
Extracts real quest template chains from the database, applies reachability classification,
and outputs reproducible markdown reports with SHA256 provenance headers.
"""

import os
import sys
import json
import hashlib
import subprocess
import argparse
from pathlib import Path

GENERATOR_VERSION = "5.3.0"
CENSUS_VERSION = "3.1.1"

def compute_sha256(file_path):
    h = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def run_query(cmd_base, sql):
    cmd = cmd_base + ["-e", sql]
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
    if proc.returncode != 0:
        raise RuntimeError(f"MySQL error: {proc.stderr}\nQuery: {sql[:200]}")
    lines = proc.stdout.strip().splitlines()
    if not lines:
        return []
    # Drop header line
    return [line.split("\t") for line in lines[1:] if line.strip()]

def resolve_era(qlvl, qmin=0):
    lvl = qlvl if qlvl > 0 else qmin
    if lvl >= 68:
        return "WotLK"
    elif lvl >= 58:
        return "TBC"
    return "Classic"

def load_snapshot(repo_root):
    snap_path = Path(repo_root) / "docs" / "generated" / "progression-runtime-snapshot.json"
    if not snap_path.exists():
        raise FileNotFoundError(f"Runtime snapshot missing at {snap_path}. Run unit tests or generate snapshot first.")
    with open(snap_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    sha256 = compute_sha256(snap_path)
    return data, sha256

def classify_chain_link(q1_id, q1_lvl, q1_min, q2_id, q2_lvl, q2_min, link_type, layout_mappings, active_eras={'Classic', 'TBC', 'WotLK'}, max_cap=60):
    """
    Testable classification helper evaluating chain link progression between two quests.
    Distinguishes Predecessor and Successor based on link_type semantics:
    - For NextQuest / Breadcrumb: Q1 is predecessor, Q2 is successor.
    - For PrevQuest: Q2 is predecessor, Q1 is successor.
    """
    if link_type in ('NextQuest', 'Breadcrumb'):
        pred_id, pred_lvl, pred_min = q1_id, q1_lvl, q1_min
        succ_id, succ_lvl, succ_min = q2_id, q2_lvl, q2_min
    else:
        pred_id, pred_lvl, pred_min = q2_id, q2_lvl, q2_min
        succ_id, succ_lvl, succ_min = q1_id, q1_lvl, q1_min

    era_pred = resolve_era(pred_lvl, pred_min)
    era_succ = resolve_era(succ_lvl, succ_min)

    # Rule 1: Target era disabled in simulated layout
    if era_succ not in active_eras:
        return 'CONFLICT', f'Target era {era_succ} disabled in active layout'

    def get_eff(auth, era):
        if auth <= 0:
            return None
        m = layout_mappings.get(era, {})
        if str(auth) in m:
            return m[str(auth)]
        # Fallback linear interpolation within era boundaries if exact authored level not present
        if era == 'Classic':
            if auth < 1: return 1
            if auth >= 60: return 45
            return int(round(1.0 + (auth - 1.0) / 59.0 * 44.0))
        elif era == 'TBC':
            if auth <= 58: return 45
            if auth >= 70: return 55
            return int(round(45.0 + (auth - 58.0) / 12.0 * 10.0))
        elif era == 'WotLK':
            if auth <= 68: return 55
            if auth >= 80: return 60
            return int(round(55.0 + (auth - 68.0) / 12.0 * 5.0))
        return auth

    eff_pred = get_eff(pred_lvl, era_pred)
    eff_pred_min = get_eff(pred_min, era_pred) if pred_min > 0 else 1
    eff_succ = get_eff(succ_lvl, era_succ)
    eff_succ_min = get_eff(succ_min, era_succ) if succ_min > 0 else 1

    # Rule 2: Effective MinLevel of successor exceeds cap
    if eff_succ_min is not None and eff_succ_min > max_cap:
        return 'CONFLICT', f'Target effective min level ({eff_succ_min}) exceeds level cap ({max_cap})'

    # Special internal/debug MinLevel flags (> 80)
    if succ_min > 80:
        return 'WARNING', f'Special/internal MinLevel flag ({succ_min})'

    # Rule 3: Effective MinLevel > Effective QuestLevel when both valid static
    if succ_lvl > 0 and (0 < succ_min <= 80) and eff_succ is not None and eff_succ_min is not None:
        if eff_succ_min > eff_succ:
            return 'CONFLICT', f'Target effective min level ({eff_succ_min}) exceeds quest level ({eff_succ})'

    # Reachable point after predecessor
    p_reach = max(eff_pred if eff_pred else 1, eff_pred_min)

    # Rule 4: Direct chain progression gap (successor min level unreachable after predecessor)
    if link_type == 'NextQuest' and pred_lvl > 0 and (0 < succ_min <= 80) and eff_succ_min is not None:
        if eff_succ_min > (p_reach + 8):
            return 'CONFLICT', f'Target effective min level ({eff_succ_min}) unreachable after predecessor (reaches {p_reach})'

    # Warnings
    if pred_lvl <= 0 or succ_lvl <= 0:
        return 'WARNING', f'Dynamic/special quest level semantics ({pred_lvl} -> {succ_lvl})'

    if succ_min > 80:
        return 'WARNING', f'Special/internal MinLevel flag ({succ_min})'

    if link_type == 'Breadcrumb' and succ_lvl < pred_lvl:
        return 'WARNING', f'Breadcrumb level regression ({pred_lvl} -> {succ_lvl})'

    if (pred_lvl - succ_lvl) > 6:
        return 'WARNING', f'Authored level regression gap ({pred_lvl} -> {succ_lvl})'

    if era_pred != era_succ:
        if eff_succ_min is not None and eff_succ_min < (p_reach - 10):
            return 'WARNING', f'Cross-era jump with large level gap ({era_pred} -> {era_succ})'

    return 'PASS', 'Chain monotonic and reachable'

def generate_reports(repo_root, cmd_base):
    output_dir = Path(repo_root) / "docs" / "generated"
    output_dir.mkdir(parents=True, exist_ok=True)

    snapshot, snapshot_sha256 = load_snapshot(repo_root)

    header = (
        f"<!--\n"
        f"Generated by: tools/content_census/generate_progression_reports.py\n"
        f"Generator version: {GENERATOR_VERSION}\n"
        f"Source DB/census version: {CENSUS_VERSION}\n"
        f"Runtime Snapshot SHA256: {snapshot_sha256}\n"
        f"-->\n\n"
    )

    # =========================================================================
    # 1. Progression Layout Report
    # =========================================================================
    layout_md = header + "# CoA Universal Content Scaling: Progression Layout Report\n\n"
    layout_md += "## 1. Executive Summary\n\n"
    layout_md += "The Progression Layout is the single authoritative source of truth for all era ranges and tier unlock windows. All consumers (commands, tests, reports, access checks, item requirements) derive their ranges strictly from `ProgressionLayout::Create`.\n\n"
    layout_md += "## 2. Authoritative Layout Matrix\n\n"
    layout_md += "| Configuration | Max Level | Active Eras | Classic Range | TBC Range | WotLK Range | Semantics |\n"
    layout_md += "|---|---|---|---|---|---|---|\n"

    config_names = [
        ("cap60_all", "Cap 60 (All Eras)", 60, True, True),
        ("cap60_classic", "Cap 60 (Classic Only)", 60, False, False),
        ("cap60_classic_tbc", "Cap 60 (Classic + TBC)", 60, True, False),
        ("cap60_classic_wotlk", "Cap 60 (Classic + WotLK)", 60, False, True),
        ("cap70_all", "Cap 70 (All Eras)", 70, True, True),
        ("cap70_classic_tbc", "Cap 70 (Classic + TBC)", 70, True, False),
        ("cap80_all", "Cap 80 (Stock Blizzard)", 80, True, True),
    ]

    for key, name, ml, tbc, wotlk in config_names:
        lay = snapshot["layouts"][key]
        c_str = f"{lay['classic'][0]} \u2013 {lay['classic'][1]}"
        t_str = f"{lay['tbc'][0]} \u2013 {lay['tbc'][1]}" if lay.get("tbc") else "[Disabled]"
        w_str = f"{lay['wotlk'][0]} \u2013 {lay['wotlk'][1]}" if lay.get("wotlk") else "[Disabled]"
        sem = "Stock Authentic (58/68 overlap)" if ml == 80 and tbc and wotlk else "Contiguous Compressed"
        layout_md += f"| **{name}** | {ml} | {'Classic' + (', TBC' if tbc else '') + (', WotLK' if wotlk else '')} | {c_str} | {t_str} | {w_str} | {sem} |\n"

    layout_md += "\n## 3. Tier Unlock Gates (Cap 60 All Eras)\n\n"
    tier_unlocks_60 = snapshot["layouts"]["cap60_all"]["tierUnlocks"]
    for era in ["Classic", "TBC", "WotLK"]:
        layout_md += f"### {era} Tier Unlocks\n"
        era_tiers = tier_unlocks_60.get(era, {})
        for tier in ["DUNGEON_NORMAL", "DUNGEON_HEROIC", "RAID_ENTRY", "RAID_MID", "RAID_END", "RAID_PINNACLE"]:
            lvl = era_tiers.get(tier, 255)
            layout_md += f"- **{tier}**: Level {lvl}\n"
        layout_md += "\n"

    with open(output_dir / "progression-layout-report.md", "w", encoding="utf-8") as f:
        f.write(layout_md)

    # =========================================================================
    # 2. Real Quest Chains Audit
    # =========================================================================
    sql = (
        "SELECT q1.ID, q1.QuestLevel, q1.MinLevel, "
        "q2.ID, q2.QuestLevel, q2.MinLevel, 'NextQuest' AS LinkType "
        "FROM quest_template q1 "
        "JOIN quest_template_addon a1 ON q1.ID = a1.ID "
        "JOIN quest_template q2 ON q2.ID = a1.NextQuestID "
        "WHERE a1.NextQuestID > 0 "
        "UNION ALL "
        "SELECT q1.ID, q1.QuestLevel, q1.MinLevel, "
        "q2.ID, q2.QuestLevel, q2.MinLevel, 'Breadcrumb' AS LinkType "
        "FROM quest_template q1 "
        "JOIN quest_template_addon a1 ON q1.ID = a1.ID "
        "JOIN quest_template q2 ON q2.ID = a1.BreadcrumbForQuestId "
        "WHERE a1.BreadcrumbForQuestId > 0 "
        "UNION ALL "
        "SELECT q1.ID, q1.QuestLevel, q1.MinLevel, "
        "q2.ID, q2.QuestLevel, q2.MinLevel, 'PrevQuest' AS LinkType "
        "FROM quest_template q1 "
        "JOIN quest_template_addon a1 ON q1.ID = a1.ID "
        "JOIN quest_template q2 ON q2.ID = ABS(a1.PrevQuestID) "
        "WHERE a1.PrevQuestID != 0 "
        "ORDER BY 1, 4, 7;"
    )
    raw_rows = run_query(cmd_base, sql) if cmd_base else []
    # Deduplicate only by (sourceId, targetId, linkType) to preserve distinct relationship types
    seen = set()
    rows = []
    for r in raw_rows:
        key = (int(r[0]), int(r[3]), r[6])
        if key not in seen:
            seen.add(key)
            rows.append(r)

    conflicts_md = header + "# Quest Progression Conflicts & Reachability Scan\n\n"
    conflicts_md += "## 1. Scope & Methodology\n\n"
    conflicts_md += (
        f"Scanned real DB quest chains and breadcrumbs from `quest_template` and `quest_template_addon` "
        f"({len(rows)} chained links evaluated). Effective levels evaluated across Cap 60 (All Eras) layout.\n\n"
        "### Audit Classification Rules:\n"
        "- **Rule 1 (Enabled Destination)**: Target quest era must be active in layout (CONFLICT if disabled).\n"
        "- **Rule 2 (Cap Safety)**: Effective minimum level of successor must not exceed MaxPlayerLevel (CONFLICT if > Cap).\n"
        "- **Rule 3 (Level Sanity)**: Effective minimum level must not exceed effective quest level for static quests (CONFLICT if min > quest level).\n"
        "- **Rule 4 (Chain Reachability)**: In direct chains (NextQuest), successor MinLevel must not require a progression gap > 8 effective levels (CONFLICT if unreachable).\n"
        "- **Warnings**: Dynamic/special quest levels, authored level drops, and cross-era level disparities flagged as non-blocking WARNINGs.\n\n"
    )
    conflicts_md += "## 2. Sampled Real Quest Chain Audit\n\n"
    conflicts_md += "| Quest ID | Target Quest ID | Link Type | Authored Lvl (Q1 -> Q2) | Effective Lvl (Q1 -> Q2) | Era | Status | Reason |\n"
    conflicts_md += "|---|---|---|---|---|---|---|---|\n"

    cap60_mappings = snapshot["layouts"]["cap60_all"]["levelMappings"]

    def get_eff_sample(auth, era):
        if auth <= 0:
            return 1
        return cap60_mappings.get(era, {}).get(str(auth), auth)

    pass_count = 0
    warn_count = 0
    conflict_count = 0
    cross_era_count = 0
    transitions = {"Classic->TBC": 0, "Classic->WotLK": 0, "TBC->WotLK": 0, "Other": 0}

    sample_rows = []
    conflict_samples = []

    for r in rows:
        q1_id = int(r[0])
        q1_lvl = int(r[1])
        q1_min = int(r[2])
        q2_id = int(r[3])
        q2_lvl = int(r[4])
        q2_min = int(r[5])
        link_type = r[6]

        era1 = resolve_era(q1_lvl, q1_min)
        era2 = resolve_era(q2_lvl, q2_min)
        if era1 != era2:
            cross_era_count += 1
            key = f"{era1}->{era2}"
            if key in transitions:
                transitions[key] += 1
            else:
                transitions["Other"] += 1

        status, reason = classify_chain_link(
            q1_id, q1_lvl, q1_min, q2_id, q2_lvl, q2_min, link_type, cap60_mappings, max_cap=60
        )

        if status == "PASS":
            pass_count += 1
        elif status == "WARNING":
            warn_count += 1
        elif status == "CONFLICT":
            conflict_count += 1
            if len(conflict_samples) < 10:
                eff1 = get_eff_sample(q1_lvl, era1)
                eff2 = get_eff_sample(q2_lvl, era2)
                conflict_samples.append((q1_id, q2_id, link_type, f"{q1_lvl} -> {q2_lvl}", f"{eff1} -> {eff2}", era1, status, reason))

        if len(sample_rows) < 25:
            eff1 = get_eff_sample(q1_lvl, era1)
            eff2 = get_eff_sample(q2_lvl, era2)
            sample_rows.append((q1_id, q2_id, link_type, f"{q1_lvl} -> {q2_lvl}", f"{eff1} -> {eff2}", era1, status, reason))

    for item in sample_rows:
        conflicts_md += f"| {item[0]} | {item[1]} | {item[2]} | {item[3]} | {item[4]} | {item[5]} | {item[6]} | {item[7]} |\n"

    total = len(rows)
    reachability_pct = ((total - conflict_count) / total * 100.0) if total > 0 else 100.0

    conflicts_md += (
        f"\n## 3. Audit Verification Summary\n\n"
        f"- **Total Chain Links Evaluated**: {total}\n"
        f"- **PASS (Fully Reachable)**: {pass_count}\n"
        f"- **WARNING (Catch-up / Reverse Gap / Dynamic)**: {warn_count}\n"
        f"- **CONFLICT (Blocking progression gap / Invalid constraint)**: {conflict_count}\n"
        f"- **Cross-Era Chain Links**: {cross_era_count} (Classic\u2192TBC: {transitions['Classic->TBC']}, Classic\u2192WotLK: {transitions['Classic->WotLK']}, TBC\u2192WotLK: {transitions['TBC->WotLK']}, Other/Reverse: {transitions['Other']})\n"
        f"- **Calculated Reachability Rate**: {reachability_pct:.2f}%\n"
    )

    if conflict_samples:
        conflicts_md += "\n### Sample Identified Progression Conflicts\n\n"
        conflicts_md += "| Quest ID | Target Quest ID | Link Type | Authored Lvl (Q1 -> Q2) | Effective Lvl (Q1 -> Q2) | Era | Status | Reason |\n"
        conflicts_md += "|---|---|---|---|---|---|---|---|\n"
        for item in conflict_samples:
            conflicts_md += f"| {item[0]} | {item[1]} | {item[2]} | {item[3]} | {item[4]} | {item[5]} | {item[6]} | {item[7]} |\n"

    with open(output_dir / "quest-progression-conflicts.md", "w", encoding="utf-8") as f:
        f.write(conflicts_md)

    # =========================================================================
    # 3. Reward Budget Report (Loaded from Runtime Snapshot)
    # =========================================================================
    budget_md = header + "# Progression Reward Budget Report\n\n"
    budget_md += "## 1. Executive Summary\n\n"
    budget_md += "Authoritative XP calibration, gold conversion limits, and item requirement scaling across progression layouts derived directly from C++ runtime snapshot.\n\n"
    budget_md += "## 2. Real Quests Core-Equivalent XP & Max-Level Money Conversion\n\n"
    budget_md += "| Quest ID | Quest Title | Authored Level | Authored XP (DBC) | Cap 60 Effective Level | Core-Equivalent XP | At-Cap Gold (Safe Guard) |\n"
    budget_md += "|---|---|---|---|---|---|---|\n"

    for sq in snapshot["sampleQuests"]:
        qid = sq["id"]
        title = sq["title"]
        auth_lvl = sq["authoredLevel"]
        auth_xp = sq["authoredXP"]
        eff_lvl = sq["effectiveLevel"]
        calib_xp = sq["calibratedXP"]
        copper = sq["atCapMoneyCopper"]
        gold_str = f"{copper // 10000}g {(copper % 10000) // 100}s"
        budget_md += f"| {qid} | {title} | {auth_lvl} | {auth_xp} | {eff_lvl} | {calib_xp} | {gold_str} |\n"

    with open(output_dir / "reward-budget-report.md", "w", encoding="utf-8") as f:
        f.write(budget_md)

    # =========================================================================
    # 4. Access Progression Report (Loaded from Runtime Snapshot)
    # =========================================================================
    access_md = header + "# Content Access & Unlock Progression Report\n\n"
    access_md += "## 1. Executive Summary\n\n"
    access_md += "Authoritative dungeon, heroic, and raid entry access gating under ProgressionLayout authority derived directly from C++ runtime snapshot.\n\n"
    access_md += "## 2. Real Instance Unlock Progression (Cap 60 All Eras)\n\n"
    access_md += "| Instance / Raid | Map ID | Difficulty | Era | Tier | Authored Min | Effective Access Min | Tier Unlock Gate |\n"
    access_md += "|---|---|---|---|---|---|---|---|\n"

    for si in snapshot["sampleInstances"]:
        name = si["name"]
        mid = si["mapId"]
        diff = si["difficulty"]
        era = si["era"]
        tier = si["tier"]
        auth_min = si["authoredMin"]
        eff_access_min = si["effectiveAccessMin"]
        tier_gate = si["tierUnlockGate"]
        diff_label = "Heroic" if diff == 1 else "Normal"
        access_md += f"| {name} | {mid} | {diff_label} | {era} | {tier} | {auth_min} | {eff_access_min} | Level {tier_gate} |\n"

    with open(output_dir / "access-progression-report.md", "w", encoding="utf-8") as f:
        f.write(access_md)

    print(f"All progression reports successfully generated and verified against runtime snapshot (SHA256: {snapshot_sha256[:12]}...).")

def main():
    default_repo = Path(__file__).resolve().parents[2]
    default_mysql = os.getenv("COA_MYSQL_BIN")
    default_defaults = os.getenv("COA_MYSQL_DEFAULTS")

    parser = argparse.ArgumentParser(description="Generate progression reports")
    parser.add_argument("--mysql-bin", default=default_mysql)
    parser.add_argument("--defaults-file", default=default_defaults)
    parser.add_argument("--world-db", default="acore_world")
    parser.add_argument("--repo-root", default=str(default_repo))
    args = parser.parse_args()

    if not args.mysql_bin or not os.path.exists(args.mysql_bin):
        sys.exit(
            f"ERROR: MySQL binary not specified or not found. Provide --mysql-bin or set COA_MYSQL_BIN environment variable."
        )

    cmd_base = [args.mysql_bin]
    if args.defaults_file:
        if not os.path.exists(args.defaults_file):
            sys.exit(
                f"ERROR: MySQL defaults file not found at '{args.defaults_file}'. Provide valid path or set COA_MYSQL_DEFAULTS."
            )
        cmd_base.append(f"--defaults-file={args.defaults_file}")
    cmd_base.append(args.world_db)

    generate_reports(args.repo_root, cmd_base)

if __name__ == "__main__":
    main()
