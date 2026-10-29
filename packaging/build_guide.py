#!/usr/bin/env python3
"""Illustrated public calendar guide; generated deterministically at release."""
import json
import math
import sys
from pathlib import Path
from reportlab.lib.colors import HexColor
from reportlab.lib.pagesizes import A4
from reportlab.lib.utils import simpleSplit
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

W,H=A4
INK=HexColor("#23334A")
MID=HexColor("#68788A")
ACCENT=HexColor("#B45E39")
BG=HexColor("#FAF8F3")
PALE=HexColor("#EFEAE0")
WHITE=HexColor("#FFFFFF")
FONT="Helvetica";BOLD="Helvetica-Bold"
for name,filename in [("Guide","DejaVuSans.ttf"),("GuideBold","DejaVuSans-Bold.ttf")]:
    path=Path("/usr/share/fonts/truetype/dejavu")/filename
    if path.is_file():pdfmetrics.registerFont(TTFont(name,str(path)))
if "Guide" in pdfmetrics.getRegisteredFontNames():FONT,BOLD="Guide","GuideBold"
def label(c,text,x,y,size=10,bold=False,color=INK):
    c.setFont(BOLD if bold else FONT,size)
    c.setFillColor(color)
    c.drawString(x,y,str(text))
def paragraph(c,text,x,y,width,size=10,spacing=17):
    for segment in str(text).split("\n"):
        for line in simpleSplit(segment,FONT,size,width):
            label(c,line,x,y,size)
            y-=spacing
        y-=5
    return y
def page(c,n,title):
    c.setFillColor(BG);c.rect(0,0,W,H,fill=1,stroke=0)
    label(c,"INFILTRATOR  /  THE CALENDAR COLLECTION",45,H-37,8,True,MID)
    label(c,title,45,H-92,20,True)
    c.setStrokeColor(PALE);c.line(45,51,W-45,51)
    label(c,"A PICTURE GUIDE TO WORLD CALENDARS",45,32,8,False,MID)
    label(c,str(n)+" / 3",W-80,32,8,False,MID)
def moon(c,x,y,r,phase):
    c.setFillColor(INK);c.circle(x,y,r,stroke=0,fill=1)
    if phase==0:return
    c.setFillColor(WHITE)
    if phase==2:c.circle(x,y,r-1,stroke=0,fill=1)
    elif phase==1:c.wedge(x-r,y-r,x+r,y+r,-90,180,stroke=0,fill=1)
    else:c.wedge(x-r,y-r,x+r,y+r,90,180,stroke=0,fill=1)
def picture(c,data,x,y,w,h):
    kind=data["diagram"]
    c.setFillColor(PALE);c.roundRect(x,y,w,h,12,fill=1,stroke=0)
    if kind=="lunisolar":
        for i,phase in enumerate([0,1,2,3]):
            cx=x+82+i*113
            moon(c,cx,y+h-67,25,phase)
            label(c,["NEW","WAXING","FULL","WANING"][i],cx-24,y+h-109,9,True,MID)
        for i in range(12):
            dx=x+24+i*(w-48)/12
            c.setFillColor(INK if i%2 else MID)
            c.rect(dx,y+50,(w-50)/12,24,fill=1,stroke=0)
        c.setFillColor(ACCENT);c.roundRect(x+27,y+16,w-54,22,6,fill=1,stroke=0)
        label(c,"LEAP MONTH SOMETIMES ADDED TO ALIGN WITH SEASONS",x+37,y+23,8,True,WHITE)
    elif kind=="longcount":
        names=["BAK'TUN","K'ATUN","TUN","WINAL","K'IN"]
        vals=["144,000","7,200","360","20","1"]
        for i in range(5):
            yy=y+h-51-i*41
            c.setFillColor(INK if i%2 else MID)
            c.roundRect(x+24,yy-11,175,30,4,fill=1,stroke=0)
            label(c,names[i],x+36,yy-2,10,True,WHITE)
            label(c,vals[i]+" DAYS",x+230,yy-2,11,True,ACCENT)
    elif kind=="swedish":
        items=[("1700","Leap day omitted"),("1712","30 February added"),("1753","17 Feb -> 1 Mar")]
        c.setStrokeColor(ACCENT);c.setLineWidth(5);c.line(x+45,y+100,x+w-45,y+100)
        for i,(year,note) in enumerate(items):
            cx=x+85+i*(w-170)/2
            c.setFillColor(INK);c.circle(cx,y+100,9,stroke=0,fill=1)
            label(c,year,cx-22,y+124,14,True,ACCENT)
            for j,line in enumerate(simpleSplit(note,FONT,9,140)):
                label(c,line,cx-68,y+66-j*16,9)
        c.setFillColor(ACCENT);c.roundRect(x+130,y+10,w-260,26,7,fill=1,stroke=0)
        label(c,"FEBRUARY 30, 1712",x+145,y+18,9,True,WHITE)
def main():
    data=json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    out=Path(sys.argv[2]);out.parent.mkdir(parents=True,exist_ok=True)
    c=canvas.Canvas(str(out),pagesize=A4,invariant=1)
    c.setTitle(data["title"]+" - Illustrated public guide")
    c.setAuthor("Infiltrator Projects")
    page(c,1,"A DIFFERENT WAY TO COUNT TIME")
    label(c,data["title"],45,H-145,15,True,ACCENT)
    y=paragraph(c,data["intro"],45,H-178,W-90,11,20)
    picture(c,data,45,H-515,W-90,240)
    label(c,data["diagram_caption"],51,H-547,10,True,ACCENT)
    paragraph(c,data["caption_detail"],51,H-570,W-102,10,18)
    c.showPage()
    page(c,2,"HOW TO UNDERSTAND THE DATES")
    y=H-146
    for i,item in enumerate(data["steps"]):
        c.setFillColor(ACCENT);c.circle(63,y+8,15,fill=1,stroke=0)
        label(c,str(i+1),58,y+3,10,True,WHITE)
        label(c,item["heading"],94,y+5,12,True,INK)
        y=paragraph(c,item["text"],94,y-25,W-142,10.5,19)-30
    c.setFillColor(PALE);c.roundRect(45,155,W-90,108,9,fill=1,stroke=0)
    label(c,"EXAMPLE - READ THE CALENDAR",60,236,10,True,ACCENT)
    paragraph(c,data["example"],60,213,W-120,10,18)
    label(c,"The illustration is a teaching diagram, not a date converter.",45,100,9,False,MID)
    c.showPage()
    page(c,3,"HISTORY AND IMPORTANT LIMITS")
    label(c,"WHERE IT COMES FROM",45,H-147,11,True,ACCENT)
    y=paragraph(c,data["history"],45,H-176,W-90,10.5,19)
    label(c,"WHAT THIS SOFTWARE MODELS",45,y-15,11,True,ACCENT)
    y=paragraph(c,data["model"],45,y-45,W-90,10.5,19)
    label(c,"WHAT IT DOES NOT CLAIM",45,y-18,11,True,ACCENT)
    y=paragraph(c,data["limits"],45,y-48,W-90,10,18)
    label(c,"FURTHER READING",45,max(y-16,150),10,True,ACCENT)
    yy=max(y-39,125)
    for source in data["sources"]:
        yy=paragraph(c,source,45,yy,W-90,8,13)-2
    c.save()
if __name__=="__main__":main()
