#!/usr/bin/env python3
"""Host-independent tests for the M3 token source and artifact compiler."""

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOKENS = ROOT / "design" / "tokens"
COMPILER = ROOT / "tools" / "m3-token-compiler.py"
A11Y = ROOT / "tools" / "a11y-contrast.py"


def load_compiler():
    spec = importlib.util.spec_from_file_location("m3_token_compiler_tested", COMPILER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


compiler = load_compiler()


class TokenSourceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tokens = compiler.load_tokens(TOKENS)
        cls.colour = cls.tokens["colour-roles.json"]

    def test_all_token_sources_have_the_expected_families(self):
        self.assertEqual(set(self.tokens), set(compiler.TOKEN_FILES))
        type_roles = self.tokens["typography.json"]["roles"]
        expected_types = {
            "{0}{1}".format(group, size)
            for group, sizes in (
                ("display", ("Large", "Medium", "Small")),
                ("headline", ("Large", "Medium", "Small")),
                ("title", ("Large", "Medium", "Small")),
                ("body", ("Large", "Medium", "Small")),
                ("label", ("Large", "Medium", "Small")),
            )
            for size in sizes
        }
        self.assertEqual(set(type_roles), expected_types)
        self.assertEqual(self.tokens["typography.json"]["font"]["family"], "Inter")
        self.assertEqual(
            self.tokens["shape.json"]["corners"],
            {
                "none": 0, "extraSmall": 4, "small": 8, "medium": 12,
                "large": 16, "extraLarge": 28, "full": -1,
            },
        )
        self.assertEqual(self.tokens["motion.json"]["easing"]["emphasizedAccelerate"],
                         [0.3, 0, 0.8, 0.15])
        self.assertEqual(self.tokens["motion.json"]["easing"]["emphasizedDecelerate"],
                         [0.05, 0.7, 0.1, 1])
        self.assertEqual(self.tokens["elevation.json"]["levels"],
                         {"level0": 0, "level1": 1, "level2": 3,
                          "level3": 6, "level4": 8, "level5": 12})
        self.assertEqual(self.tokens["density.json"]["spacing"]["md"], 12)
        self.assertEqual(self.tokens["iconography.json"]["grid"]["size"], 24)

    def test_complete_m3_role_set_has_resolvable_modes(self):
        standard_pairs = ("primary", "secondary", "tertiary", "error")
        expected = set()
        for prefix in standard_pairs:
            expected.update({prefix, "on" + prefix[0].upper() + prefix[1:]})
            container = prefix + "Container"
            expected.update({container, "on" + container[0].upper() + container[1:]})
            capitalized = prefix[0].upper() + prefix[1:]
            expected.update({
                prefix + "Fixed", prefix + "FixedDim",
                "on" + capitalized + "Fixed", "on" + capitalized + "FixedVariant",
            }) if prefix != "error" else None
        expected.update({
            "background", "onBackground", "surface", "onSurface", "surfaceVariant",
            "onSurfaceVariant", "outline", "outlineVariant", "shadow", "scrim",
            "inverseSurface", "inverseOnSurface", "inversePrimary", "surfaceTint",
            "surfaceDim", "surfaceBright", "surfaceContainerLowest", "surfaceContainerLow",
            "surfaceContainer", "surfaceContainerHigh", "surfaceContainerHighest",
        })
        self.assertEqual(set(self.colour["roles"]), expected)
        self.assertTrue(expected <= set(self.colour["requiredRoles"]))
        for mode in ("light", "dark"):
            scheme = compiler.resolve_baseline_scheme(mode, self.colour)
            self.assertEqual(len(scheme["roles"]), len(expected))

    def test_variant_allow_list_and_source_colors(self):
        self.assertEqual(set(self.colour["variants"]), {
            "content", "expressive", "fidelity", "monochrome",
            "neutral", "tonal-spot", "vibrant",
        })
        for mode in ("light", "dark"):
            scheme = compiler.resolve_baseline_scheme(mode, self.colour)
            self.assertEqual(scheme["sourceColor"], "#6750A4")
            self.assertEqual(scheme["mode"], mode)


class CompilerTest(unittest.TestCase):
    def setUp(self):
        self.tokens = compiler.load_tokens(TOKENS)
        self.colour = self.tokens["colour-roles.json"]
        self.scheme = compiler.resolve_baseline_scheme("light", self.colour)

    def normalized(self):
        return compiler.normalize_scheme(self.scheme, self.colour)

    def test_normalization_is_canonical_and_rejects_bad_input(self):
        lowercase = dict(self.scheme)
        lowercase["sourceColor"] = "#abcdef"
        lowercase["roles"] = dict(reversed(list(self.scheme["roles"].items())))
        lowercase["roles"]["primary"] = "#abcdef"
        normalized = compiler.normalize_scheme(lowercase, self.colour)
        self.assertEqual(normalized["sourceColor"], "#ABCDEF")
        self.assertEqual(normalized["roles"]["primary"], "#ABCDEF")
        self.assertEqual(list(normalized["roles"]), list(self.colour["roles"]))

        original = dict(self.scheme)
        for mode in (None, "LIGHT", "auto", 1):
            with self.subTest(mode=mode):
                invalid = dict(original, mode=mode)
                with self.assertRaisesRegex(compiler.CompilerError, "scheme.mode"):
                    compiler.normalize_scheme(invalid, self.colour)
        for color in ("6750A4", "#123", "#12345678", "#GG0000", None, 0):
            with self.subTest(color=color):
                invalid = dict(original, sourceColor=color)
                with self.assertRaisesRegex(compiler.CompilerError, "sourceColor"):
                    compiler.normalize_scheme(invalid, self.colour)

    def test_missing_extra_and_malformed_roles_fail(self):
        missing = dict(self.scheme)
        missing["roles"] = dict(self.scheme["roles"])
        del missing["roles"]["onSurface"]
        with self.assertRaisesRegex(compiler.CompilerError, "onSurface"):
            compiler.normalize_scheme(missing, self.colour)

        unknown = dict(self.scheme)
        unknown["roles"] = dict(self.scheme["roles"], primry="#6750A4")
        with self.assertRaisesRegex(compiler.CompilerError, "primry"):
            compiler.normalize_scheme(unknown, self.colour)

        malformed = dict(self.scheme)
        malformed["roles"] = dict(self.scheme["roles"], primary="#00000")
        with self.assertRaisesRegex(compiler.CompilerError, "scheme.roles.primary"):
            compiler.normalize_scheme(malformed, self.colour)

    def test_artifacts_cover_all_roles_and_consumers(self):
        artifacts, contrast = compiler.compile_artifacts(self.scheme, self.tokens)
        self.assertEqual(set(artifacts), {
            "scheme-normalized.json", "theme.qml", "gtk.css", "xrdb",
            "qt-palette.conf", "colors.css",
        })
        normalized = json.loads(artifacts["scheme-normalized.json"])
        self.assertEqual(normalized, self.normalized())
        qml = artifacts["theme.qml"].decode("utf-8")
        self.assertIn("pragma Singleton", qml)
        self.assertIn("QtObject {", qml)
        self.assertIn("readonly property bool isDark: false", qml)
        for role, color in self.scheme["roles"].items():
            self.assertIn('readonly property color {0}: "{1}"'.format(role, color), qml)
            self.assertIn("@define-color {0} {1};".format(role, color),
                          artifacts["gtk.css"].decode("utf-8"))
            self.assertIn("*m3.{0}: {1}".format(role, color),
                          artifacts["xrdb"].decode("utf-8"))
            self.assertIn("{0}={1}".format(role, color),
                          artifacts["qt-palette.conf"].decode("utf-8"))
            self.assertIn("--m3-{0}: {1};".format(compiler.css_name(role), color),
                          artifacts["colors.css"].decode("utf-8"))
        self.assertIn("color-scheme: light;", artifacts["colors.css"].decode("utf-8"))
        self.assertTrue(all(result["passes"] for result in contrast))

    def test_determinism_is_independent_of_input_role_order(self):
        first, _ = compiler.compile_artifacts(self.scheme, self.tokens)
        reordered = dict(self.scheme, roles=dict(reversed(list(self.scheme["roles"].items()))))
        second, _ = compiler.compile_artifacts(reordered, self.tokens)
        self.assertEqual(first, second)
        self.assertNotIn(str(ROOT).encode(), b"".join(first.values()))

    def test_contrast_uses_wcag_and_blocks_required_failures(self):
        self.assertAlmostEqual(compiler.contrast_ratio("#000000", "#FFFFFF"), 21.0)
        self.assertAlmostEqual(compiler.contrast_ratio("#777777", "#FFFFFF"), 4.478089, places=6)
        invalid = dict(self.scheme)
        invalid["roles"] = dict(self.scheme["roles"], onSurface=self.scheme["roles"]["surface"])
        with self.assertRaisesRegex(compiler.CompilerError, "onSurface/surface"):
            compiler.compile_artifacts(invalid, self.tokens)

    def test_atomic_generation_and_byte_check(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "theme"
            artifacts, _ = compiler.compile_artifacts(self.scheme, self.tokens)
            for name, content in artifacts.items():
                compiler.atomic_write(output / name, content)
            self.assertEqual(compiler.check_artifacts(output, artifacts), [])
            self.assertFalse(list(output.glob("*.tmp")))
            stale = dict(artifacts)
            stale["gtk.css"] = stale["gtk.css"] + b"/* stale */\n"
            checked = compiler.check_artifacts(output, stale)
            self.assertEqual(checked, [(output / "gtk.css", "out of date")])
            (output / "xrdb").unlink()
            checked = compiler.check_artifacts(output, artifacts)
            self.assertIn((output / "xrdb", "missing"), checked)

    def test_cli_all_check_and_dark_mode(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "nested" / "theme"
            command = [sys.executable, str(COMPILER), "--all", "--tokens", str(TOKENS),
                       "--out", str(output)]
            generated = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(generated.returncode, 0, generated.stderr)
            self.assertEqual(set(path.name for path in output.iterdir()), {"light", "dark"})
            for mode in ("light", "dark"):
                self.assertEqual(len(list((output / mode).iterdir())), 6)
            qml = (output / "dark" / "theme.qml").read_text(encoding="utf-8")
            self.assertIn("readonly property bool isDark: true", qml)
            checked = subprocess.run(command + ["--check"], capture_output=True, text=True,
                                     check=False)
            self.assertEqual(checked.returncode, 0, checked.stderr)
            self.assertEqual(checked.stdout.count("OK "), 2)

    def test_check_never_writes_and_reports_missing(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "does-not-exist"
            result = subprocess.run(
                [sys.executable, str(COMPILER), "--all", "--out", str(output), "--check"],
                capture_output=True, text=True, check=False,
            )
            self.assertEqual(result.returncode, 1)
            self.assertFalse(output.exists())
            self.assertEqual(result.stdout.count("STALE "), 12)

    def test_cli_single_scheme_failure_does_not_write(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source = directory / "scheme.json"
            output = directory / "out"
            self.scheme["roles"]["onPrimary"] = self.scheme["roles"]["primary"]
            source.write_text(json.dumps(self.scheme), encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(COMPILER), "--scheme", str(source), "--out", str(output)],
                capture_output=True, text=True, check=False,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn("contrast", result.stderr)
            self.assertFalse(output.exists())

    def test_accessibility_cli_strict_and_non_strict(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "scheme.json"
            source.write_text(json.dumps(self.scheme), encoding="utf-8")
            command = [sys.executable, str(A11Y), "--scheme", str(source), "--json"]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report["passed"])
            self.assertEqual(len(report["results"]), len(self.colour["contrastPairs"]))
            strict = subprocess.run(command + ["--strict"], capture_output=True, text=True,
                                    check=False)
            self.assertEqual(strict.returncode, 0, strict.stderr)

            invalid = dict(self.scheme)
            invalid["roles"] = dict(self.scheme["roles"],
                                    onSurface=self.scheme["roles"]["surface"])
            source.write_text(json.dumps(invalid), encoding="utf-8")
            failed = subprocess.run(command + ["--strict"], capture_output=True, text=True,
                                    check=False)
            self.assertEqual(failed.returncode, 1)
            self.assertFalse(json.loads(failed.stdout)["passed"])


if __name__ == "__main__":
    unittest.main()
