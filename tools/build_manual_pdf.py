from pathlib import Path
import re

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    Image,
    KeepTogether,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
)

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs" / "MANUAL.md"
OUTPUT = ROOT / "docs" / "2E0LXY-LoRa-APRS-iGate-Manual.pdf"

NAVY = colors.HexColor("#061321")
SURFACE = colors.HexColor("#0A1C2C")
BLUE = colors.HexColor("#2C8CFF")
CYAN = colors.HexColor("#16C8F5")
GREEN = colors.HexColor("#65D66E")
AMBER = colors.HexColor("#FFB020")
TEXT = colors.HexColor("#152638")
MUTED = colors.HexColor("#5D7285")
LINE = colors.HexColor("#BCD1E1")


def esc(text: str) -> str:
    text = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    text = re.sub(r"`([^`]+)`", r"<font name='Courier'>\1</font>", text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    return text


styles = getSampleStyleSheet()
styles.add(ParagraphStyle(
    name="CoverTitle", parent=styles["Title"], fontName="Helvetica-Bold",
    fontSize=28, leading=32, textColor=colors.white, alignment=TA_CENTER,
    spaceAfter=10,
))
styles.add(ParagraphStyle(
    name="CoverSub", parent=styles["BodyText"], fontSize=12, leading=18,
    textColor=colors.HexColor("#B9D4EC"), alignment=TA_CENTER,
))
styles.add(ParagraphStyle(
    name="H1x", parent=styles["Heading1"], fontName="Helvetica-Bold",
    fontSize=18, leading=22, textColor=NAVY, spaceBefore=11, spaceAfter=7,
))
styles.add(ParagraphStyle(
    name="H2x", parent=styles["Heading2"], fontName="Helvetica-Bold",
    fontSize=13, leading=17, textColor=BLUE, spaceBefore=8, spaceAfter=5,
))
styles.add(ParagraphStyle(
    name="Bodyx", parent=styles["BodyText"], fontName="Helvetica",
    fontSize=9.3, leading=13.5, textColor=TEXT, spaceAfter=6,
))
styles.add(ParagraphStyle(
    name="Bulletx", parent=styles["Bodyx"], leftIndent=13, firstLineIndent=-7,
    bulletIndent=5, spaceAfter=3,
))
styles.add(ParagraphStyle(
    name="Smallx", parent=styles["Bodyx"], fontSize=7.8, leading=10,
    textColor=MUTED,
))


def page_decor(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setFillColor(NAVY)
    canvas.rect(0, height - 15 * mm, width, 15 * mm, fill=1, stroke=0)
    canvas.setFillColor(CYAN)
    canvas.setFont("Helvetica-Bold", 9)
    canvas.drawString(16 * mm, height - 9.5 * mm, "2E0LXY LoRa APRS iGate")
    canvas.setFillColor(colors.white)
    canvas.setFont("Helvetica", 8)
    canvas.drawRightString(width - 16 * mm, height - 9.5 * mm, "Heltec V3.2 - Firmware v1.1.5")
    canvas.setStrokeColor(LINE)
    canvas.line(16 * mm, 13 * mm, width - 16 * mm, 13 * mm)
    canvas.setFillColor(MUTED)
    canvas.setFont("Helvetica", 7.5)
    canvas.drawString(16 * mm, 8 * mm, "github.com/2E0LXY/2E0LXY-LoRa-APRS-iGate")
    canvas.drawRightString(width - 16 * mm, 8 * mm, f"Page {doc.page}")
    canvas.restoreState()


doc = BaseDocTemplate(
    str(OUTPUT), pagesize=A4,
    rightMargin=16 * mm, leftMargin=16 * mm,
    topMargin=21 * mm, bottomMargin=18 * mm,
    title="2E0LXY LoRa APRS iGate and Digipeater Manual",
    author="2E0LXY",
)
frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="normal")
doc.addPageTemplates(PageTemplate(id="manual", frames=frame, onPage=page_decor))

story = []
cover = Table(
    [[Paragraph("2E0LXY", styles["CoverTitle"])],
     [Paragraph("LoRa APRS iGate and Digipeater", styles["CoverTitle"])],
     [Paragraph("Installation, configuration and operation manual", styles["CoverSub"])],
     [Spacer(1, 5 * mm)],
     [Paragraph("Heltec WiFi LoRa 32 V3.2", styles["CoverSub"])],
     [Paragraph("UK 439.9125 MHz profile - Firmware v1.1.5 - July 2026", styles["CoverSub"])]],
    colWidths=[doc.width],
    rowHeights=[18 * mm, 17 * mm, 12 * mm, 10 * mm, 8 * mm, 8 * mm],
)
cover.setStyle(TableStyle([
    ("BACKGROUND", (0, 0), (-1, -1), NAVY),
    ("BOX", (0, 0), (-1, -1), 1, BLUE),
    ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
    ("LEFTPADDING", (0, 0), (-1, -1), 12 * mm),
    ("RIGHTPADDING", (0, 0), (-1, -1), 12 * mm),
]))
story.extend([Spacer(1, 19 * mm), cover, Spacer(1, 10 * mm)])

hero = ROOT / "images" / "2e0lxy-configuration.png"
if hero.exists():
    im = Image(str(hero), width=doc.width, height=doc.width * 0.5625)
    story.extend([im, Spacer(1, 5 * mm)])
story.append(Paragraph(
    "Copyright © 2026 2E0LXY for original modifications. "
    "2E0LXY LoRa APRS iGate. GNU GPLv3 - supplied without warranty.",
    styles["Smallx"],
))
story.append(Spacer(1, 15 * mm))

lines = SOURCE.read_text(encoding="utf-8").splitlines()[3:]
i = 0
while i < len(lines):
    line = lines[i].strip()
    if not line:
        i += 1
        continue
    if line.startswith("## "):
        story.append(Paragraph(esc(line[3:]), styles["H1x"]))
    elif line.startswith("### "):
        story.append(Paragraph(esc(line[4:]), styles["H2x"]))
    elif line.startswith("- "):
        story.append(Paragraph("• " + esc(line[2:]), styles["Bulletx"]))
    elif re.match(r"^\d+\. ", line):
        num, text = line.split(". ", 1)
        story.append(Paragraph(f"<b>{num}.</b> {esc(text)}", styles["Bulletx"]))
    elif line.startswith("|") and i + 1 < len(lines) and set(lines[i + 1].replace("|", "").replace(" ", "")) <= {"-", ":"}:
        table_lines = [line]
        i += 2
        while i < len(lines) and lines[i].strip().startswith("|"):
            table_lines.append(lines[i].strip())
            i += 1
        i -= 1
        data = []
        for row in table_lines:
            cells = [c.strip() for c in row.strip("|").split("|")]
            data.append([Paragraph(esc(c), styles["Smallx"]) for c in cells])
        col_widths = [doc.width / len(data[0])] * len(data[0])
        tbl = Table(data, colWidths=col_widths, repeatRows=1)
        tbl.setStyle(TableStyle([
            ("BACKGROUND", (0, 0), (-1, 0), SURFACE),
            ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
            ("GRID", (0, 0), (-1, -1), 0.5, LINE),
            ("VALIGN", (0, 0), (-1, -1), "TOP"),
            ("LEFTPADDING", (0, 0), (-1, -1), 6),
            ("RIGHTPADDING", (0, 0), (-1, -1), 6),
            ("TOPPADDING", (0, 0), (-1, -1), 5),
            ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
        ]))
        story.extend([KeepTogether(tbl), Spacer(1, 3 * mm)])
    else:
        paragraph = line
        while i + 1 < len(lines):
            nxt = lines[i + 1].strip()
            if not nxt or nxt.startswith(("#", "-", "|")) or re.match(r"^\d+\. ", nxt):
                break
            i += 1
            paragraph += " " + nxt
        story.append(Paragraph(esc(paragraph), styles["Bodyx"]))
    i += 1

doc.build(story)
print(OUTPUT)
