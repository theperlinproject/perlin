#!/usr/bin/env python3
"""Exact RGB pixel comparison. Reject differently sized inputs; no tolerance/resizing."""
import argparse,json,sys
from pathlib import Path
from PIL import Image,ImageChops

def main():
    p=argparse.ArgumentParser();p.add_argument('expected',type=Path);p.add_argument('actual',type=Path);p.add_argument('--diff',type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    ref=Image.open(a.expected).convert('RGB');got=Image.open(a.actual).convert('RGB')
    if ref.size!=got.size:sys.exit(f'Unmatched dimensions: {ref.size} versus {got.size}')
    diff=ImageChops.difference(ref,got)
    # Count any differing RGB channel, including a one-value difference.
    mask=ImageChops.lighter(ImageChops.lighter(*diff.split()[:2]),diff.split()[2]);hist=mask.histogram();total=ref.width*ref.height;bad=total-hist[0]
    report={'expected':a.expected.name,'actual':a.actual.name,'width':ref.width,'height':ref.height,'total_pixels':total,'different_pixels':bad,'exact_match':bad==0,'max_channel_delta':max(i for i,n in enumerate(hist) if n)}
    if a.diff:diff.save(a.diff)
    if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report));sys.exit(0 if bad==0 else 1)
if __name__=='__main__':main()
