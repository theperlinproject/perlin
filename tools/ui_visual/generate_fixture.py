#!/usr/bin/env python3
"""Generate original synthetic UI pack and independent, hand-specified pixel oracles.
No game textures or screenshots are included. The expected pixels are defined here
by explicit rectangles/patterns, never by reading a Perlin capture.
"""
import argparse, json, math
from pathlib import Path
from PIL import Image, ImageDraw

W,H=320,180
BG=(20,24,32); BLUE=(65,181,245); GREEN=(64,210,123); ORANGE=(246,157,54); PINK=(224,85,173)
COLORS=[[(255,88,88),(243,188,69),(87,203,108)],[(54,165,223),(51,66,88),(184,112,242)],[(240,106,178),(95,220,217),(255,240,180)]]
TILE=[[(255,106,106),(255,210,70)],[(75,174,245),(90,214,132)]]
STRIP=[(232,96,86),(100,215,130),(90,150,245)]

def main():
    p=argparse.ArgumentParser();p.add_argument('root',type=Path);p.add_argument('--expected',type=Path,required=True);args=p.parse_args()
    root=args.root; tex=root/'textures/ui';ui=root/'ui';tex.mkdir(parents=True,exist_ok=True);ui.mkdir(parents=True,exist_ok=True);args.expected.mkdir(parents=True,exist_ok=True)
    def texture(name,w,h,fn):
        im=Image.new('RGB',(w,h));im.putdata([fn(x,y) for y in range(h) for x in range(w)]);im.save(tex/(name+'.png'))
    texture('white',8,8,lambda x,y:(255,255,255))
    texture('frame',16,24,lambda x,y: COLORS[0 if y<6 else 2 if y>=18 else 1][0 if x<4 else 2 if x>=12 else 1]);(tex/'frame.json').write_text(json.dumps({'base_size':[8,8],'nineslice_size':[2,2]},indent=2)+'\n')
    texture('tile',4,4,lambda x,y:TILE[y//2][x//2]);texture('strip',12,4,lambda x,y:STRIP[x//4]);texture('gray',8,8,lambda x,y:(220,220,220))
    controls=[];draw=[]
    def rect(x,y,w,h,color):draw.append(('rect',(x,y,w,h,color)))
    def control(name,kind,x,y,w,h,**props):return {name:{'type':kind,'anchor_from':'top_left','anchor_to':'top_left','offset':[x,y],'size':[w,h],**props}}
    def sprite(name,x,y,w,h,texture='white',color=None,**props):
        if color is not None:props['color']=[c/255 for c in color]
        controls.append(control(name,'image',x,y,w,h,texture='textures/ui/'+texture,**({'keep_ratio':False}|props)))
    sprite('background',0,0,W,H,color=BG,layer=-100);rect(0,0,W,H,BG)
    # Five directional clips, plus 0%, 100%, and a non-integer texel cut.
    for i,(direction,ratio) in enumerate([('left',.25),('right',.25),('up',.5),('down',.5),('center',.5),('left',0),('left',1),('right',.3)]):
        x=8+i*39;y=12;w=32;h=24
        sprite('clip_bg_'+str(i),x,y,w,h,color=(39,48,62));rect(x,y,w,h,(39,48,62))
        sprite('clip_'+str(i),x,y,w,h,color=BLUE,clip_direction=direction,clip_ratio=ratio)
        n=math.floor(ratio*8)/8
        if direction=='left':rect(x,y,w*(1-n),h,BLUE)
        elif direction=='right':rect(x+w*n,y,w*(1-n),h,BLUE)
        elif direction=='up':rect(x,y,w,h*(1-n),BLUE)
        elif direction=='down':rect(x,y+h*n,w,h*(1-n),BLUE)
        else:rect(x+w*n/2,y+h*n/2,w*(1-n),h*(1-n),BLUE)
    # Non-square texture sidecar: 4/6 physical texels become two logical units.
    sprite('frame',8,54,66,38,'frame');
    for j,(y,h) in enumerate([(54,2),(56,34),(90,2)]):
        for i,(x,w) in enumerate([(8,2),(10,62),(72,2)]):rect(x,y,w,h,COLORS[j][i])
    sprite('tiles',84,54,50,38,'tile',tiled='xy',tiled_scale=[2,3]);
    for y in range(38):
        for x in range(50):rect(84+x,54+y,1,1,TILE[(y%12)//6][(x%8)//4])
    # 3:1 texture: contain = 36x12, cover = middle crop of square.
    sprite('contain',146,54,36,36,'strip',keep_ratio=True)
    for i,c in enumerate(STRIP):rect(146+i*12,66,12,12,c)
    sprite('cover',194,54,36,36,'strip',fill=True);rect(194,54,36,36,STRIP[1])
    sprite('uv',242,54,32,36,'strip',uv=[4,0],uv_size=[4,4]);rect(242,54,32,36,STRIP[1])
    # Fractional logical positions are snapped only at physical-pixel scale.
    sprite('fractional',286.5,54.5,21,36,color=ORANGE);rect(286.5,54.5,21,36,ORANGE)
    # Deeply nested clipping: each parent shifts the left and top edges in by two.
    node=control('clipped_image','image',-30,-30,110,110,texture='textures/ui/white',keep_ratio=False,color=[c/255 for c in GREEN])
    for i in reversed(range(9)):
        node=control('clip_level_'+str(i),'panel',8 if i==0 else 2,118 if i==0 else 2,75-i*4,48-i*4,clips_children=True,controls=[node])
    controls.append(node);rect(24,134,43,16,GREEN)
    # A clipped group has no private z-order: an external sibling belongs between its children.
    controls.append(control('layer_parent','panel',98,118,80,48,clips_children=True,controls=[control('behind','image',0,0,50,34,texture='textures/ui/white',keep_ratio=False,color=[c/255 for c in PINK],layer=-2),control('front','image',24,12,50,34,texture='textures/ui/white',keep_ratio=False,color=[c/255 for c in ORANGE],layer=2)]))
    sprite('between',112,126,48,26,color=BLUE,layer=1)
    rect(98,118,50,34,PINK);rect(112,126,48,26,BLUE);rect(122,130,50,34,ORANGE)
    # 50% opacity on opaque background; oracle uses exact 8-bit premultiplied blending.
    sprite('opacity',190,118,54,48,color=(240,160,80),alpha=.5)
    rect(190,118,54,48,(130,92,56))
    sprite('bound_clip',256,118,52,48,color=BLUE,clip_direction='left',bindings=[{'binding_type':'global','binding_name':'#test_clip','binding_name_override':'#clip_ratio'}]);rect(256,118,26,48,BLUE)
    # The diagnostic variant adds labels; its bitmap glyphs are not compared to Bedrock.
    labels=[]
    for name,x,y,text in [('header',8,1,'CLIP: LEFT / RIGHT / UP / DOWN / CENTER / ZERO / FULL / TEXEL'),('frame_label',8,42,'NINE SLICE'),('tile_label',84,42,'XY TILE'),('contain_label',146,42,'CONTAIN'),('cover_label',194,42,'COVER'),('uv_label',242,42,'UV'),('frac_label',286,42,'SNAP'),('deep_label',8,106,'9 CLIPS'),('layer_label',98,106,'LAYERS'),('opacity_label',190,106,'ALPHA'),('binding_label',256,106,'BINDING')]:
        labels.append(control(name,'label',x,y,310 if name=='header' else 80,10,text=text,font_scale_factor=.75,layer=20,color=[.86,.9,.96],shadow=False))
    screen={'type':'screen','controls':controls};(ui/'visual.json').write_text(json.dumps({'namespace':'parity','pixels':screen,'diagnostic':{'type':'screen','controls':controls+labels}},indent=2)+'\n');(ui/'_ui_defs.json').write_text(json.dumps({'ui_defs':['ui/visual.json']})+'\n');(root/'state.json').write_text(json.dumps({'bindings':{'#test_clip':.5}})+'\n')
    (root/'manifest.json').write_text(json.dumps({'format_version':2,'header':{'name':'Perlin synthetic UI parity fixture','description':'Original synthetic test textures, not a vanilla screen replacement','uuid':'d28b5153-3c92-4a4d-9719-77d3ecda2100','version':[1,0,0],'min_engine_version':[1,21,0]},'modules':[{'type':'resources','uuid':'03eab7a6-6681-4527-85c7-df8c67dbe07b','version':[1,0,0]}]},indent=2)+'\n')
    for scale in (1,2,3):
        image=Image.new('RGB',(W*scale,H*scale));d=ImageDraw.Draw(image)
        rnd=lambda x:math.floor(x*scale+.5)
        for _,(x,y,w,h,c) in draw:
            if rnd(x+w)>rnd(x) and rnd(y+h)>rnd(y):d.rectangle((rnd(x),rnd(y),rnd(x+w)-1,rnd(y+h)-1),fill=c)
        image.save(args.expected/f'expected-{scale}x.png')
if __name__=='__main__':main()
