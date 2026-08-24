import subprocess
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.core.write_manifest import command_output  # noqa: E402


class ManifestTests(unittest.TestCase):
    @patch("analysis.core.write_manifest.subprocess.run")
    def test_empty_git_status_means_clean_tree(self, run):
        run.return_value = subprocess.CompletedProcess([], 0, stdout="", stderr="")
        self.assertEqual(command_output("git", "status", "--porcelain"), "")
        self.assertFalse(bool(command_output("git", "status", "--porcelain")))


if __name__ == "__main__":
    unittest.main()
