#!/usr/bin/env python3
"""
jASTERIX release check.

Checks a packaged jasterix_client (AppImage, or a plain binary) together with a definitions
package (zip, or a folder), the pairing that is shipped to users.

Two groups of input:
- samples: every *.bin in the samples folder (the checked-in test recordings), the edition is
  taken from the file name cat<NNN>ed<edition>[_variant].bin
- recordings: files matched by the entries of a settings JSON in the recordings folder

Per file three runs are made with the same record limit: --analyze, a structured decode and
a flat decode. All three are reduced to the same statistics (records and data sources per
category, count/min/max per item path). The analysis runs on the flat columns, so it must
match the flat decode exactly. Structured and flat are compared on their common paths. The
analysis is also compared to the stored result of an earlier run (--store writes it).

For PCAP files the analysis runs per network stream, so with a record limit it does not see
the same records as the decodes. It is then compared to the stored result only.

Exit code 0 when every check passed, 1 otherwise.
"""

import argparse
import fnmatch
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

ANALYSIS_MARKER = "analysis result:"
COUNTER_KEYS = ("num_frames", "num_records", "num_errors", "num_ref_errors", "num_spf_errors")
SETTINGS_NAME = "jasterix_release_check_recordings.json"
EXPECTED_NAME = "jasterix_release_check_expected.json"

# columns written next to the item leaves in flat mode, not record content
SIDE_COLUMNS = {"artas_md5", "record_data", "recording_time", "recording_day", "recording_date"}

# samples with settings that do not follow from the file name
SAMPLE_SETTINGS = {
    "ioss_truncated_hexdump.bin": {"framing": "ioss", "editions": "2:1.0", "expect_errors": True},
    "cat021ed2.4_spf_aireon.bin": {"spf_editions": "21:Aireon"},
    "cat010ed0.24_sensis.bin": {"editions": "10:0.24_sensis"},
}

# flat mode applies two CAT001 corrections that structured output does not have: SAC/SIC
# propagation into records without I001/010, and the full Time of Day reconstruction
FLAT_ONLY_PATHS = {"1": {"010.SAC", "010.SIC", "140.Time-of-Day"}}


# ----------------------------------------------------------------------------------------------
# running the client

class Client:
    def __init__(self, executable, definition_path, work_dir):
        self.executable = executable
        self.definition_path = definition_path
        self.work_dir = work_dir
        self.run_count = 0

    def common_args(self, settings, record_limit):
        args = [self.executable, "--definition_path", self.definition_path]

        if settings.get("pcap"):
            args.append("--pcap")
        elif settings.get("framing"):
            args += ["--framing", settings["framing"]]

        for option, key in (("--editions", "editions"), ("--ref_edition", "ref_editions"),
                            ("--spf_edition", "spf_editions")):
            if settings.get(key):
                args += [option, settings[key]]

        if record_limit and record_limit > 0:
            args += ["--record_limit", str(record_limit)]

        return args

    def run(self, args, timeout):
        self.run_count += 1
        started = time.time()
        proc = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              timeout=timeout)
        output = proc.stdout.decode("utf-8", errors="replace")
        return proc.returncode, output, time.time() - started

    def analyze(self, filename, settings, record_limit, timeout):
        args = self.common_args(settings, record_limit) + ["--filename", filename, "--analyze"]
        code, output, seconds = self.run(args, timeout)

        if code != 0:
            raise RuntimeError("analyze exit code {}:\n{}".format(code, tail(output)))

        pos = output.find(ANALYSIS_MARKER)
        if pos < 0:
            raise RuntimeError("no analysis result in output:\n{}".format(tail(output)))

        text = output[pos + len(ANALYSIS_MARKER):]
        start = text.find("{")
        end = text.rfind("}")
        if start < 0 or end < 0:
            raise RuntimeError("no JSON in analysis output:\n{}".format(tail(output)))

        return json.loads(text[start:end + 1]), seconds

    def decode(self, filename, settings, record_limit, flat, timeout):
        out_path = os.path.join(self.work_dir, "decode_{}_{}.json".format(
            self.run_count, "flat" if flat else "structured"))

        args = self.common_args(settings, record_limit) + [
            "--filename", filename, "--write_type", "text", "--write_filename", out_path,
            "--print_indent", "-1", "--log_perf"]
        if flat:
            args.append("--flat")

        code, output, seconds = self.run(args, timeout)

        if code != 0:
            raise RuntimeError("decode exit code {}:\n{}".format(code, tail(output)))

        counters = parse_counters(output)

        chunks = []
        if os.path.exists(out_path):
            with open(out_path, "r") as f:
                for line in f:
                    line = line.strip()
                    if line:
                        chunks.append(json.loads(line))
            os.remove(out_path)

        return chunks, counters, seconds


def tail(text, lines=15):
    return "\n".join(text.strip().splitlines()[-lines:])


def parse_counters(output):
    """frames and records from the --log_perf summary of the client, plus the number of
    ERROR log lines (the client prints no error count after decoding)"""
    counters = {}
    m = re.search(r"decoded (\d+) frames, (\d+) records", output)
    if m:
        counters["num_frames"] = int(m.group(1))
        counters["num_records"] = int(m.group(2))
    counters["log_errors"] = sum(1 for line in output.splitlines() if line.startswith("ERROR"))
    return counters


# ----------------------------------------------------------------------------------------------
# reduction to statistics
#
# stats = {
#   "cats": { "<cat>": { "records": n, "sensors": {"<sac>/<sic>": n}, "items": { "<path>": {count, min, max} } } },
#   "counters": { num_frames, num_records, ... },
#   "skipped_categories": {...}
# }

def new_stats():
    return {"cats": {}, "counters": {}, "skipped_categories": {}}


def cat_stats(stats, cat):
    return stats["cats"].setdefault(str(cat), {"records": 0, "sensors": {}, "items": {}})


def is_primitive(value):
    return value is None or isinstance(value, (bool, int, float, str))


def update_item(items, path, value):
    entry = items.setdefault(path, {"count": 0})
    entry["count"] += 1

    # min/max like the analyze mode: only for primitive values, compared within one type
    if not is_primitive(value) or value is None or isinstance(value, bool):
        return

    if "min" not in entry:
        entry["min"] = value
        entry["max"] = value
        return

    if type(entry["min"]) is type(value) or (
            isinstance(entry["min"], (int, float)) and isinstance(value, (int, float))):
        if value < entry["min"]:
            entry["min"] = value
        if value > entry["max"]:
            entry["max"] = value


def walk_record(items, prefix, obj):
    """same traversal as jASTERIX::addJSONAnalysis: objects recurse, everything else counts"""
    for key, value in obj.items():
        path = prefix + "." + key if prefix else key
        if isinstance(value, dict):
            walk_record(items, path, value)
        else:
            update_item(items, path, value)


def sensor_of(record):
    item = record.get("010")
    if isinstance(item, dict) and "SAC" in item and "SIC" in item:
        return "{}/{}".format(item["SAC"], item["SIC"])
    return "unknown"


def reduce_structured(chunks):
    stats = new_stats()

    for chunk in chunks:
        if "frames" in chunk:
            data_blocks = []
            for frame in chunk["frames"]:
                content = frame.get("content", {})
                data_blocks += content.get("data_blocks", [])
        else:
            data_blocks = chunk.get("data_blocks", [])

        for data_block in data_blocks:
            cat = data_block.get("category")
            content = data_block.get("content", {})
            if "records" not in content:
                continue

            cs = cat_stats(stats, cat)
            for record in content["records"]:
                cs["records"] += 1
                sensor = sensor_of(record)
                cs["sensors"][sensor] = cs["sensors"].get(sensor, 0) + 1
                walk_record(cs["items"], "", record)

    return stats


def reduce_flat(chunks):
    stats = new_stats()

    for chunk in chunks:
        for cat, columns in chunk.items():
            if not cat.isdigit() or not isinstance(columns, dict):
                continue

            cs = cat_stats(stats, cat)

            num_records = max((len(col) for col in columns.values() if isinstance(col, list)),
                              default=0)
            cs["records"] += num_records

            sac_col = columns.get("010.SAC", [])
            sic_col = columns.get("010.SIC", [])
            for idx in range(num_records):
                sac = sac_col[idx] if idx < len(sac_col) else None
                sic = sic_col[idx] if idx < len(sic_col) else None
                sensor = "unknown" if sac is None or sic is None else "{}/{}".format(sac, sic)
                cs["sensors"][sensor] = cs["sensors"].get(sensor, 0) + 1

            for path, col in columns.items():
                if not isinstance(col, list):
                    continue
                for value in col:
                    if value is not None:
                        update_item(cs["items"], path, value)

    return stats


def merge_stats(target, source):
    """adds the records, data sources, items and counters of source into target"""
    for key, value in source["counters"].items():
        target["counters"][key] = target["counters"].get(key, 0) + value

    for cat, skipped in source["skipped_categories"].items():
        merged = target["skipped_categories"].setdefault(cat, dict(skipped))
        if merged is not skipped:
            for key in ("data_blocks", "bytes"):
                if key in skipped:
                    merged[key] = merged.get(key, 0) + skipped[key]

    for cat, sc in source["cats"].items():
        tc = cat_stats(target, cat)
        tc["records"] += sc["records"]
        for sensor, count in sc["sensors"].items():
            tc["sensors"][sensor] = tc["sensors"].get(sensor, 0) + count
        for path, entry in sc["items"].items():
            merged = tc["items"].setdefault(path, {"count": 0})
            merged["count"] += entry["count"]
            for bound, pick in (("min", min), ("max", max)):
                if bound in entry:
                    if bound in merged and same_kind(merged[bound], entry[bound]):
                        merged[bound] = pick(merged[bound], entry[bound])
                    elif bound not in merged:
                        merged[bound] = entry[bound]


def reduce_analysis(result):
    # PCAP analysis: one complete analysis per network stream, merged into one
    streams = [value for value in result.values()
               if isinstance(value, dict) and any(key in value for key in COUNTER_KEYS)]
    if streams:
        stats = new_stats()
        for stream in streams:
            merge_stats(stats, reduce_single_analysis(stream))
        return stats

    return reduce_single_analysis(result)


def reduce_single_analysis(result):
    stats = new_stats()

    for key, value in result.items():
        if key in COUNTER_KEYS:
            stats["counters"][key] = value
        elif key == "skipped_categories":
            stats["skipped_categories"] = value
        elif isinstance(value, dict):
            sensor = key
            for cat, entries in value.items():
                cs = cat_stats(stats, cat)
                for path, entry in entries.items():
                    if path == "count":
                        cs["records"] += entry
                        cs["sensors"][sensor] = cs["sensors"].get(sensor, 0) + entry
                        continue

                    merged = cs["items"].setdefault(path, {"count": 0})
                    merged["count"] += entry.get("count", 0)
                    for bound, pick in (("min", min), ("max", max)):
                        if bound in entry:
                            if bound in merged and same_kind(merged[bound], entry[bound]):
                                merged[bound] = pick(merged[bound], entry[bound])
                            elif bound not in merged:
                                merged[bound] = entry[bound]

    return stats


def same_kind(a, b):
    if isinstance(a, bool) or isinstance(b, bool):
        return type(a) is type(b)
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return True
    return type(a) is type(b)


# ----------------------------------------------------------------------------------------------
# comparison

def values_equal(a, b, tolerance):
    if isinstance(a, (int, float)) and isinstance(b, (int, float)) and not isinstance(a, bool) \
            and not isinstance(b, bool):
        if a == b:
            return True
        return math.isclose(a, b, rel_tol=tolerance, abs_tol=tolerance)
    return a == b


def compare_stats(left, right, left_name, right_name, tolerance, strict_paths=True,
                  compare_sensors=True, skip_cat1_sensors=False, ignore_paths=(),
                  cat_ignore_paths=None):
    """returns a list of difference strings, empty when equal"""
    diffs = []
    cat_ignore_paths = cat_ignore_paths or {}

    cats = sorted(set(left["cats"]) | set(right["cats"]), key=int)
    for cat in cats:
        lc = left["cats"].get(cat)
        rc = right["cats"].get(cat)
        if lc is None or rc is None:
            diffs.append("cat {}: only in {}".format(cat, left_name if lc else right_name))
            continue

        if lc["records"] != rc["records"]:
            diffs.append("cat {}: records {} {} vs {} {}".format(
                cat, left_name, lc["records"], right_name, rc["records"]))

        if compare_sensors and not (skip_cat1_sensors and cat == "1"):
            if lc["sensors"] != rc["sensors"]:
                diffs.append("cat {}: data sources differ: {} {} vs {} {}".format(
                    cat, left_name, sorted(lc["sensors"].items()),
                    right_name, sorted(rc["sensors"].items())))

        ignored = set(ignore_paths) | cat_ignore_paths.get(cat, set())
        left_paths = set(lc["items"]) - ignored
        right_paths = set(rc["items"]) - ignored

        if strict_paths:
            for path in sorted(left_paths - right_paths):
                diffs.append("cat {}: item {} only in {}".format(cat, path, left_name))
            for path in sorted(right_paths - left_paths):
                diffs.append("cat {}: item {} only in {}".format(cat, path, right_name))

        for path in sorted(left_paths & right_paths):
            le = lc["items"][path]
            re_ = rc["items"][path]
            if le["count"] != re_["count"]:
                diffs.append("cat {}: item {} count {} {} vs {} {}".format(
                    cat, path, left_name, le["count"], right_name, re_["count"]))
            for bound in ("min", "max"):
                if bound in le and bound in re_ and not values_equal(le[bound], re_[bound], tolerance):
                    diffs.append("cat {}: item {} {} {} {} vs {} {}".format(
                        cat, path, bound, left_name, le[bound], right_name, re_[bound]))

    return diffs


def compare_counters(left, right, left_name, right_name, keys):
    diffs = []
    for key in keys:
        if key in left and key in right and left[key] != right[key]:
            diffs.append("{} {} {} vs {} {}".format(key, left_name, left[key], right_name, right[key]))
    return diffs


# ----------------------------------------------------------------------------------------------
# file discovery

def sample_settings(name):
    settings = dict(SAMPLE_SETTINGS.get(name, {}))
    m = re.match(r"cat(\d+)ed(\d+(?:\.\d+)*)(?:_|\.bin$)", name)
    if m and "editions" not in settings:
        settings["editions"] = "{}:{}".format(int(m.group(1)), m.group(2))
    return settings


def find_samples(samples_dir):
    files = []
    for name in sorted(os.listdir(samples_dir)):
        if name.endswith(".bin"):
            files.append(("samples/" + name, os.path.join(samples_dir, name), sample_settings(name)))
    return files


def find_recordings(recordings_dir, config):
    """files of every glob entry, with the optional per-file "file_settings" merged in"""
    files = []
    file_settings = config.get("file_settings", {})
    for entry in config.get("recordings", []):
        pattern = entry["glob"]
        entry_settings = {k: v for k, v in entry.items() if k != "glob"}
        matched = []
        for root, _, names in os.walk(recordings_dir):
            for name in names:
                rel = os.path.relpath(os.path.join(root, name), recordings_dir)
                if fnmatch.fnmatch(rel, pattern):
                    matched.append(rel)
        if not matched:
            print("WARNING: no file matches '{}'".format(pattern))
        for rel in sorted(matched):
            settings = dict(entry_settings)
            settings.update(file_settings.get(rel, {}))
            files.append((rel, os.path.join(recordings_dir, rel), settings))
    return files


# ----------------------------------------------------------------------------------------------
# main

def check_file(client, key, path, settings, record_limit, expected, tolerance, timeout):
    """runs the three modes and all comparisons, returns (ok, lines, analysis stats)"""
    lines = []
    ok = True
    is_sample = key.startswith("samples/")
    limit = 0 if is_sample else settings.get("record_limit", record_limit)

    analysis, t_analyze = client.analyze(path, settings, limit, timeout)
    structured, counters_s, t_structured = client.decode(path, settings, limit, False, timeout)
    flat, counters_f, t_flat = client.decode(path, settings, limit, True, timeout)

    a_stats = reduce_analysis(analysis)
    s_stats = reduce_structured(structured)
    f_stats = reduce_flat(flat)
    s_stats["counters"] = counters_s
    f_stats["counters"] = counters_f

    num_records = a_stats["counters"].get("num_records", 0)
    num_errors = a_stats["counters"].get("num_errors", 0)
    cats = ",".join(sorted(a_stats["cats"], key=int))

    lines.append("  analyze {:.1f} s, structured {:.1f} s, flat {:.1f} s, records {}, errors {}, "
                 "cats {}".format(t_analyze, t_structured, t_flat, num_records, num_errors, cats))

    num_ref_errors = a_stats["counters"].get("num_ref_errors", 0)
    num_spf_errors = a_stats["counters"].get("num_spf_errors", 0)
    max_errors = settings.get("max_errors", 0)

    if settings.get("expect_errors"):
        if num_errors == 0:
            ok = False
            lines.append("  FAIL: errors expected, none reported")
    elif num_errors > max_errors or num_ref_errors or num_spf_errors:
        ok = False
        lines.append("  FAIL: errors {} (allowed {}) ref errors {} spf errors {}".format(
            num_errors, max_errors, num_ref_errors, num_spf_errors))
    elif num_errors:
        lines.append("  info: errors {} within the allowed {}".format(num_errors, max_errors))

    if num_records == 0 and not settings.get("expect_errors"):
        ok = False
        lines.append("  FAIL: no records")

    log_errors = counters_s.get("log_errors", 0) + counters_f.get("log_errors", 0)
    if log_errors and not settings.get("expect_errors"):
        ok = False
        lines.append("  FAIL: {} ERROR line(s) logged by the decodes".format(log_errors))

    # files that expect errors cannot be cross-compared: analyze stops at the first error,
    # structured output keeps the failed record, flat output drops it
    if settings.get("expect_errors"):
        lines.append("  info: errors expected, modes not compared to each other")
        return ok, lines, a_stats

    # analyze vs flat: the analysis runs on the flat columns, so both must match exactly
    # (not for PCAP with a limit, the analysis runs per network stream)
    if not (settings.get("pcap") and limit):
        diffs = compare_stats(a_stats, f_stats, "analyze", "flat", tolerance,
                              ignore_paths=SIDE_COLUMNS)
        diffs += compare_counters(a_stats["counters"], f_stats["counters"], "analyze",
                                  "flat", ("num_records",))
        if diffs:
            ok = False
            lines.append("  FAIL: analyze vs flat, {} difference(s)".format(len(diffs)))
            lines += ["    " + d for d in diffs[:20]]
    else:
        lines.append("  info: PCAP with record limit, analyze compared to stored only")

    # structured vs flat: records and data sources strict (CAT001 gets SAC/SIC propagated in
    # flat mode), items compared where both have the path
    diffs = compare_stats(s_stats, f_stats, "structured", "flat", tolerance, strict_paths=False,
                          skip_cat1_sensors=True, ignore_paths=SIDE_COLUMNS,
                          cat_ignore_paths=FLAT_ONLY_PATHS)
    diffs += compare_counters(s_stats["counters"], f_stats["counters"], "structured", "flat",
                              ("num_records", "num_errors"))
    if diffs:
        ok = False
        lines.append("  FAIL: structured vs flat, {} difference(s)".format(len(diffs)))
        lines += ["    " + d for d in diffs[:20]]

    # analyze vs stored
    if expected is not None:
        stored = expected.get(key)
        if stored is None:
            ok = False
            lines.append("  FAIL: no stored result, run with --store")
        else:
            diffs = compare_stats(stored, a_stats, "stored", "analyze", tolerance)
            diffs += compare_counters(stored["counters"], a_stats["counters"], "stored", "analyze",
                                      COUNTER_KEYS)
            if stored.get("skipped_categories", {}) != a_stats.get("skipped_categories", {}):
                diffs.append("skipped categories stored {} vs analyze {}".format(
                    stored.get("skipped_categories", {}), a_stats.get("skipped_categories", {})))
            if diffs:
                ok = False
                lines.append("  FAIL: analyze vs stored, {} difference(s)".format(len(diffs)))
                lines += ["    " + d for d in diffs[:20]]

    return ok, lines, a_stats


def main():
    repo_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--appimage", help="jASTERIX client AppImage to check")
    parser.add_argument("--client", help="jasterix_client binary to check instead of an AppImage")
    parser.add_argument("--definitions_zip", help="definitions package (zip)")
    parser.add_argument("--definitions", help="definitions folder instead of a zip")
    parser.add_argument("--samples", default=os.path.join(repo_dir, "src", "test"),
                        help="folder with the checked-in *.bin samples (default: src/test)")
    parser.add_argument("--recordings", default="/home/sk/data/test",
                        help="folder with the test recordings and the settings / stored JSON")
    parser.add_argument("--settings", help="settings JSON (default: <recordings>/" + SETTINGS_NAME + ")")
    parser.add_argument("--expected", help="stored results JSON (default: <recordings>/" + EXPECTED_NAME + ")")
    parser.add_argument("--store", action="store_true",
                        help="write the analysis results as the stored results instead of checking")
    parser.add_argument("--record_limit", type=int,
                        help="records per run for recordings (default from the settings JSON, else 10000)")
    parser.add_argument("--only", help="only files whose key contains this text")
    parser.add_argument("--no_samples", action="store_true", help="skip the checked-in samples")
    parser.add_argument("--no_recordings", action="store_true", help="skip the recordings")
    parser.add_argument("--tolerance", type=float, default=1e-9, help="relative tolerance for min/max")
    parser.add_argument("--timeout", type=int, default=1800, help="seconds per client run")
    parser.add_argument("--work_dir", help="folder for temporary files (default: a temp folder)")
    args = parser.parse_args()

    if bool(args.appimage) == bool(args.client):
        parser.error("give exactly one of --appimage or --client")
    if bool(args.definitions_zip) == bool(args.definitions):
        parser.error("give exactly one of --definitions_zip or --definitions")

    work_dir = args.work_dir or tempfile.mkdtemp(prefix="jasterix_release_check_")
    os.makedirs(work_dir, exist_ok=True)
    cleanup = args.work_dir is None

    try:
        if args.definitions_zip:
            definition_path = os.path.join(work_dir, "definitions")
            with zipfile.ZipFile(args.definitions_zip) as zf:
                zf.extractall(definition_path)
            print("definitions unpacked from {}".format(args.definitions_zip))
        else:
            definition_path = args.definitions

        executable = os.path.abspath(args.appimage or args.client)
        client = Client(executable, definition_path, work_dir)
        print("client {}".format(executable))

        settings_path = args.settings or os.path.join(args.recordings, SETTINGS_NAME)
        expected_path = args.expected or os.path.join(args.recordings, EXPECTED_NAME)

        config = {}
        if not args.no_recordings:
            with open(settings_path, "r") as f:
                config = json.load(f)

        record_limit = args.record_limit or config.get("record_limit", 10000)

        expected = None
        if not args.store:
            if os.path.exists(expected_path):
                with open(expected_path, "r") as f:
                    expected = json.load(f)
            else:
                print("WARNING: no stored results at {}, run with --store to create them".format(
                    expected_path))
                expected = {}

        files = []
        if not args.no_samples:
            files += find_samples(args.samples)
        if not args.no_recordings:
            files += find_recordings(args.recordings, config)
        if args.only:
            files = [f for f in files if args.only in f[0]]

        print("record limit {}, {} file(s)\n".format(record_limit, len(files)))

        results = {}
        failed = []

        for key, path, settings in files:
            print(key)
            try:
                ok, lines, a_stats = check_file(client, key, path, settings, record_limit, expected,
                                                args.tolerance, args.timeout)
                results[key] = a_stats
            except Exception as e:  # noqa: BLE001 - report and continue with the next file
                ok, lines = False, ["  FAIL: {}".format(e)]

            for line in lines:
                print(line)
            print("  " + ("OK" if ok else "FAIL"))
            if not ok:
                failed.append(key)

        if args.store:
            stored = {}
            if os.path.exists(expected_path):
                with open(expected_path, "r") as f:
                    stored = json.load(f)
            stored.update(results)
            with open(expected_path, "w") as f:
                json.dump(stored, f, indent=1, sort_keys=True)
            print("\nstored results for {} file(s) in {}".format(len(results), expected_path))

        print("\n{} file(s), {} failed".format(len(files), len(failed)))
        for key in failed:
            print("  " + key)

        return 1 if failed else 0

    finally:
        if cleanup:
            shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
