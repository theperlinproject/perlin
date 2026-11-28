#!/usr/bin/env python3
"""Locate exact UI property strings and probable RIP-relative LEA references.
Static evidence only: presence and references do not prove behavioral semantics.
"""
import argparse, json, re
from pathlib import Path
from analyze_minecraft_static import PE

KEYS = ('clip_ratio','clip_direction','clip_pixelperfect','nineslice_size','tiled','tiled_scale','uv','uv_size','keep_ratio','fill','base_size','bilinear','grayscale','clips_children','allow_debug_missing_texture','layer','font_scale_factor','text_alignment','#clip-ratio','#clip_ratio')

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('output',type=Path);a=p.parse_args();pe=PE(a.exe)
    result={k:{'strings':[],'lea_candidates':[]} for k in KEYS};targets={}
    for sec in pe.sections:
        if sec['flags']&0x20000000:continue
        start=sec['file_offset'];end=start+sec['file_size'];data=pe.data
        for k in KEYS:
            needle=k.encode()+b'\0';i=start
            while (i:=data.find(needle,i,end))>=0:
                # Reject suffixes of other ASCII tokens.
                if i==start or data[i-1] not in b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#-':
                    rva=sec['rva']+i-start;result[k]['strings'].append(hex(rva));targets.setdefault(rva,[]).append(k)
                i+=len(needle)
    for sec in pe.sections:
        if not sec['flags']&0x20000000:continue
        for src,dst in pe.lea_refs(sec['rva'],sec['rva']+sec['file_size']):
            if dst not in targets:continue
            fn=pe.function_containing(src)
            for k in targets[dst]:result[k]['lea_candidates'].append({'instruction_rva':hex(src),'string_rva':hex(dst),'runtime_function_start_rva':hex(fn[0]) if fn else None})
    out={'method':'Exact ASCII NUL-terminated property strings and x64 RIP-relative LEA byte-pattern candidates; not a full disassembly or validated execution trace.','sha256':pe.metadata['sha256'],'file_versions':pe.metadata['file_versions'],'image_base':pe.metadata['image_base'],'properties':result}
    a.output.write_text(json.dumps(out,indent=2)+'\n')
    for k,v in result.items():print(k,len(v['strings']),len(v['lea_candidates']))
if __name__=='__main__':main()
