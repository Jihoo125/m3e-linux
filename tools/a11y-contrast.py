#!/usr/bin/env python3
"""Audit WCAG contrast for every configured Material 3 role pair."""

import argparse
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
COMPILER_PATH = ROOT / "tools" / "m3-token-compiler.py"


def load_compiler():
    """Load the sibling compiler without making tools a Python package."""
    import importlib.util

    spec = importlib.util.spec_from_file_location("m3_token_compiler", COMPILER_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load {0}".format(COMPILER_PATH))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build_parser():
    parser = argparse.ArgumentParser(
        description="Check all required and optional M3 foreground/background pairs."
    )
    parser.add_argument("--scheme", type=Path, required=True, help="scheme JSON to audit")
    parser.add_argument(
        "--tokens", type=Path, default=ROOT / "design" / "tokens",
        help="token directory (default: %(default)s)",
    )
    parser.add_argument(
        "--strict", action="store_true",
        help="also fail when optional non-text/large-text pairs miss 3:1",
    )
    parser.add_argument("--json", action="store_true", dest="json_output", help="emit JSON report")
    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)
    try:
        compiler = load_compiler()
        tokens = compiler.load_tokens(args.tokens)
        scheme = compiler.normalize_scheme(
            compiler.read_json(args.scheme), tokens["colour-roles.json"]
        )
        results = compiler.audit_scheme(scheme, tokens["colour-roles.json"])
    except (compiler.CompilerError, RuntimeError) as error:
        print("a11y-contrast: error: {0}".format(error), file=sys.stderr)
        return 2
    failed = [result for result in results if (result["required"] or args.strict) and not result["passes"]]
    if args.json_output:
        print(json.dumps({
            "mode": scheme["mode"],
            "strict": args.strict,
            "passed": not failed,
            "results": results,
        }, indent=2, sort_keys=True))
    else:
        for result in results:
            status = "PASS" if result["passes"] else "FAIL"
            severity = "required" if result["required"] else "optional"
            print(
                "{0} {1} on {2}: {3:.2f}:1 (minimum {4:.2f}:1, {5})".format(
                    status, result["foreground"], result["background"],
                    result["ratio"], result["minimum"], severity,
                )
            )
    print(
        "{0}: {1} pairs, {2} failure(s)".format(
            "FAIL" if failed else "PASS", len(results), len(failed)
        ),
        file=sys.stderr if failed or args.json_output else sys.stdout,
    )
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
