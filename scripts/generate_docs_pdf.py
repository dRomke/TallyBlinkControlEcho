#!/usr/bin/env python3
"""Generate docs/WirelessANC.pdf from docs/ARCHITECTURE.md."""

from __future__ import annotations

import re
import sys
from pathlib import Path

try:
    from fpdf import FPDF
except ImportError:
    print("FAIL: pip install fpdf2", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
MD_PATH = ROOT / "docs" / "ARCHITECTURE.md"
PDF_PATH = ROOT / "docs" / "WirelessANC.pdf"


class DocPDF(FPDF):
    def header(self) -> None:
        self.set_font("Helvetica", "I", 9)
        self.set_text_color(100, 100, 100)
        self.cell(0, 8, "WirelessANC - Architecture & Internals", align="R")
        self.ln(10)

    def footer(self) -> None:
        self.set_y(-15)
        self.set_font("Helvetica", "I", 8)
        self.set_text_color(120, 120, 120)
        self.cell(0, 10, f"Page {self.page_no()}", align="C")


_BOX_MAP = str.maketrans({
    "\u250c": "+", "\u2510": "+", "\u2514": "+", "\u2518": "+",
    "\u251c": "+", "\u2524": "+", "\u252c": "+", "\u2534": "+",
    "\u253c": "+", "\u2500": "-", "\u2502": "|",
    "\u2550": "=", "\u2551": "|",
    "\u25ba": ">", "\u25c4": "<",
})


def ascii_safe(text: str) -> str:
    text = text.translate(_BOX_MAP)
    return (
        text.replace("\u2014", "-")
        .replace("\u2013", "-")
        .replace("\u2192", "->")
        .replace("\u2190", "<-")
        .replace("\u00d7", "x")
        .replace("\u2026", "...")
    )


def strip_md(text: str) -> str:
    text = re.sub(r"`([^`]+)`", r"\1", text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"\1", text)
    text = re.sub(r"\*([^*]+)\*", r"\1", text)
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)
    return ascii_safe(text)


def write_para(pdf: DocPDF, height: float, text: str, *, fill: bool = False) -> None:
    pdf.set_x(pdf.l_margin)
    pdf.multi_cell(0, height, text, fill=fill)


def render_markdown(pdf: DocPDF, md: str) -> None:
    in_code = False
    code_lines: list[str] = []

    for raw_line in md.splitlines():
        line = raw_line.rstrip()

        if line.startswith("```"):
            if in_code:
                pdf.set_font("Courier", "", 8)
                pdf.set_fill_color(245, 245, 245)
                block = "\n".join(code_lines)
                write_para(pdf, 4.5, block, fill=True)
                pdf.ln(2)
                code_lines = []
                in_code = False
            else:
                in_code = True
            continue

        if in_code:
            code_lines.append(ascii_safe(line))
            continue

        if not line.strip():
            pdf.ln(3)
            continue

        if line.startswith("# "):
            pdf.ln(4)
            pdf.set_font("Helvetica", "B", 16)
            pdf.set_text_color(20, 20, 20)
            write_para(pdf, 9, strip_md(line[2:].strip()))
            pdf.ln(2)
            continue

        if line.startswith("## "):
            pdf.ln(3)
            pdf.set_font("Helvetica", "B", 13)
            pdf.set_text_color(30, 30, 30)
            write_para(pdf, 8, strip_md(line[3:].strip()))
            pdf.ln(1)
            continue

        if line.startswith("### "):
            pdf.ln(2)
            pdf.set_font("Helvetica", "B", 11)
            pdf.set_text_color(40, 40, 40)
            write_para(pdf, 7, strip_md(line[4:].strip()))
            continue

        if line.startswith("|") and "|" in line[1:]:
            cells = [c.strip() for c in line.strip("|").split("|")]
            if all(set(c) <= set("-: ") for c in cells):
                continue
            pdf.set_font("Helvetica", "", 7)
            pdf.set_text_color(30, 30, 30)
            row = " | ".join(strip_md(c) for c in cells)
            write_para(pdf, 4, row)
            continue

        if line.startswith("- "):
            pdf.set_font("Helvetica", "", 10)
            pdf.set_text_color(30, 30, 30)
            write_para(pdf, 5.5, f"  -  {strip_md(line[2:].strip())}")
            continue

        pdf.set_font("Helvetica", "", 10)
        pdf.set_text_color(30, 30, 30)
        write_para(pdf, 5.5, strip_md(line))


def main() -> int:
    if not MD_PATH.is_file():
        print(f"FAIL: missing {MD_PATH}", file=sys.stderr)
        return 1

    md = MD_PATH.read_text(encoding="utf-8")
    pdf = DocPDF()
    pdf.set_auto_page_break(auto=True, margin=18)
    pdf.set_margins(18, 18, 18)
    pdf.add_page()
    render_markdown(pdf, md)
    PDF_PATH.parent.mkdir(parents=True, exist_ok=True)
    pdf.output(str(PDF_PATH))
    print(f"OK: wrote {PDF_PATH}")
    return 0


if __name__ == "__main__":
    sys.exit(main())