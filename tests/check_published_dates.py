# SPDX-License-Identifier: GPL-3.0-or-later
"""Daily published-reference dates and independently enumerated period checks."""
from pathlib import Path
import json,sys
from check_provider import Module

root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'SOURCE-MANIFEST.json').read_text())
reference=json.loads((root/'tests/published-months.json').read_text())
module=Module(sys.argv[1],'calendar');months=reference['months'];first=reference['first_jdn'];last=reference['last_jdn']
if months and isinstance(months[0],list):months=[dict(zip(['jdn','year','month','leap','length'],row)) for row in months]
count=0;navigation=0;formatting=0
for index,month in enumerate(months):
    start=month['jdn'];end=months[index+1]['jdn'] if index+1<len(months) else last+1
    year=month['year']
    for j in range(max(first,start),min(last+1,end)):
        expected=[True,(year-4)%60+1,month['month'],j-start+1,(year+2636)//60+1,month['leap']]
        actual=module.observe(dict(op='fields',args=[j]));assert actual==expected,(j,expected,actual);count+=1
    for day in [1,month['length']]:
        j=start+day-1
        if not first<=j<=last:continue
        # The first record can start before the published Gregorian window.
        if first<=start:
            actual=module.observe(dict(op='start',args=[j]));assert actual==[True,start],(j,actual,start);navigation+=1
        for amount in [-25,-13,-1,0,1,13,25]:
            target=index+amount
            # Exclude the incomplete final reference month from clamping tests.
            if not 0<=target<len(months)-1:continue
            target_month=months[target];expected=target_month['jdn']+min(day,target_month['length'])-1
            if not first<=expected<=last:continue
            actual=module.observe(dict(op='periods',args=[j,amount]));assert actual==[True,expected],(j,amount,expected,actual);navigation+=1
        for amount in [-1,0,1]:
            y=year+amount;m=month['month'];leap=month['leap']
            targets=[v for v in months[:-1] if (v['year'],v['month'],v['leap'])==(y,m,leap)]
            if not targets and leap:
                m+=1
                if m==13:m=1;y+=1
                targets=[v for v in months[:-1] if (v['year'],v['month'],v['leap'])==(y,m,False)]
            if not targets:continue
            target=targets[0];expected=target['jdn']+min(day,target['length'])-1
            if not first<=expected<=last:continue
            actual=module.observe(dict(op='years',args=[j,amount]));assert actual==[True,expected],(j,amount,expected,actual);navigation+=1
        # English numeric day formatting must use the same chronology.
        text=module.observe(dict(op='calendar_format',args=[j,1,512]));expected=str(day)
        assert text==[True,expected,len(expected)],(j,expected,text);formatting+=1
print(f'{manifest["id"]}: {count} published-reference daily dates, {navigation} navigation and {formatting} date-format checks passed')
