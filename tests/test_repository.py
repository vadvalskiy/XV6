from pathlib import Path
import re
import unittest
ROOT = Path(__file__).resolve().parents[1]
XV6 = ROOT / "xv6"
class RepositoryTests(unittest.TestCase):
    def test_single_cumulative_tree(self) -> None:
        self.assertTrue((XV6 / "Makefile").is_file())
        self.assertFalse(list(ROOT.glob("labs/*/xv6")))
    def test_all_phase_regressions_are_built(self) -> None:
        text = (XV6 / "Makefile").read_text()
        for program in ("_lab1test", "_lab2test", "_schedverify", "_scounttest", "_pctest", "_rwtest", "_tickettest"):
            self.assertIn(program, text)
    def test_syscall_numbers_are_unique(self) -> None:
        text = (XV6 / "syscall.h").read_text()
        nums = re.findall(r"^#define\s+SYS_\w+\s+(\d+)\s*$", text, re.M)
        self.assertEqual(len(nums), len(set(nums)))
    def test_console_completion_covers_later_phases(self) -> None:
        text = (XV6 / "console.c").read_text()
        for command in ("lab2test", "schedverify", "scounttest", "tickettest", "workload_short"):
            self.assertIn(f'"{command}"', text)
        self.assertIn("#define MAX_FILENAME_LEN (DIRSIZ + 1)", text)
    def test_cumulative_feature_sources_exist(self) -> None:
        for name in ("find_sum.c", "sort_kernel.c", "schedstat.h", "pctest.c", "rwtest.c", "tickettest.c"):
            self.assertTrue((XV6 / name).is_file(), name)
if __name__ == "__main__": unittest.main()
