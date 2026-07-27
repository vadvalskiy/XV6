from pathlib import Path
import re
import unittest
ROOT = Path(__file__).resolve().parents[1]
XV6 = ROOT / "xv6"
class RepositoryTests(unittest.TestCase):
    def test_single_cumulative_tree(self) -> None:
        self.assertTrue((XV6 / "Makefile").is_file())
        self.assertFalse(list(ROOT.glob("labs/*/xv6")))

    def test_all_lab_documents_are_present(self) -> None:
        for lab in range(1, 5):
            slug = f"lab-{lab:02d}"
            self.assertTrue((ROOT / "docs" / "labs" / f"{slug}.md").is_file())
            self.assertTrue((ROOT / "docs" / "assignments" / slug / "assignment.fa.pdf").is_file())
            self.assertTrue((ROOT / "docs" / "assignments" / slug / "assignment.fa.txt").is_file())
            self.assertTrue((ROOT / "docs" / "reports" / slug / "report.fa.md").is_file())
            self.assertTrue((ROOT / "docs" / "reports" / slug / "report.fa.pdf").is_file())


    def test_public_documentation_layout_is_minimal(self) -> None:
        for name in (
            "README.fa.md",
            "CHANGELOG.md",
            "CODE_OF_CONDUCT.md",
            "CONTRIBUTING.md",
            "SECURITY.md",
            "NOTICE.md",
            "CITATION.cff",
            ".mailmap",
            "reports",
        ):
            self.assertFalse((ROOT / name).exists(), name)
        for name in (
            "README.md",
            "CHANGELOG.md",
            "CODE_OF_CONDUCT.md",
            "CONTRIBUTING.md",
            "SECURITY.md",
            "NOTICE.md",
        ):
            self.assertTrue((ROOT / "docs" / name).is_file(), name)
        self.assertFalse((ROOT / "docs" / "maintenance").exists())
        self.assertFalse((ROOT / "docs" / "standards").exists())

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

    def test_counter_matrix_quotes_dash_prefixed_cflags(self) -> None:
        text = (ROOT / "scripts" / "run_counter_matrix.sh").read_text()
        self.assertIn('"--extra-cflags=-DSYSCALL_COUNT_MODE=$mode"', text)
        self.assertNotIn('--extra-cflags "-DSYSCALL_COUNT_MODE=$mode"', text)
if __name__ == "__main__": unittest.main()
