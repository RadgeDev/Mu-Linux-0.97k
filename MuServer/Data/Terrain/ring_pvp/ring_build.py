import struct, math, os, hashlib, collections

KEY=[0xD1,0x73,0x52,0xF6,0xD2,0x9A,0xCB,0x27,0x3E,0xAF,0x59,0x31,0x37,0xB3,0xE7,0xA2]
def dec(buf):
    w=0x5E; out=bytearray(len(buf))
    for i,e in enumerate(buf):
        out[i]=((e^KEY[i&15])-w)&0xFF; w=(e+0x3D)&0xFF
    return out
def enc(buf):
    w=0x5E; out=bytearray(len(buf))
    for i,e in enumerate(buf):
        c=((e+w)&0xFF)^KEY[i&15]; out[i]=c; w=(c+0x3D)&0xFF
    return bytes(out)

VAN_ATT='/home/ubuntu/Mu-Linux-0.97k/MuServer/Data/Terrain/Terrain1_BACKUP_ORIGINAL.att'
VAN_OBJ='/tmp/ringbuild/vanilla_EncTerrain1.obj'
OUT='/tmp/ringbuild/out'
os.makedirs(OUT,exist_ok=True)

CX,CY=141.0,128.0
Z=165.0
R=4.0
INNER,OUTER=3.4,4.8
GATE_HALF=15.0
FENCE_MODEL=0x53
LIGHT_MODEL=0x5a
FENCE_ANGLES=[32,58,122,148,212,238,302,328]
LIGHTS=[(146.6,128.0),(141.0,133.6),(135.4,128.0),(141.0,122.4)]
REMOVE_R=2.3

def ang_of(x,y):
    return math.degrees(math.atan2(y-CY,x-CX))%360.0
def dist_of(x,y):
    return math.hypot(x-CX,y-CY)
def near_gate(a):
    for g in (0.0,90.0,180.0,270.0):
        if abs((a-g+180.0)%360.0-180.0)<=GATE_HALF: return True
    return False

# ---------- ATT ----------
van=open(VAN_ATT,'rb').read()
assert len(van)==65539 and van[:3]==b'\x00\xff\xff'
grid=bytearray(van[3:])
new=bytearray(grid)
changed=[]
for y in range(118,141):
    for x in range(130,156):
        i=y*256+x
        old=grid[i]; v=old
        d=dist_of(x,y)
        if d<=3.4: v=old & ~0x04
        if INNER<=d<=OUTER and not near_gate(ang_of(x,y)): v=old | 0x04
        if v!=old:
            new[i]=v; changed.append((x,y,old,v))
att_new=van[:3]+bytes(new)
open(f'{OUT}/Terrain1.att','wb').write(att_new)
print(f'ATT changed cells: {len(changed)}')
print(f'ATT sha256: {hashlib.sha256(att_new).hexdigest()}')

# ---------- OBJ ----------
raw=open(VAN_OBJ,'rb').read()
d=bytearray(dec(raw))
assert enc(d)==raw
n=struct.unpack_from('<H',d,2)[0]
assert 4+n*30==len(d)
recs=[]
for i in range(n):
    o=4+i*30
    rec=bytes(d[o:o+30])
    mid=struct.unpack_from('<H',d,o)[0]
    x,y,z=struct.unpack_from('<fff',d,o+2)
    recs.append((mid,x/100.0,y/100.0,rec))

removed=[(i,mid,px,py) for i,(mid,px,py,_) in enumerate(recs) if dist_of(px,py)<=REMOVE_R]
print(f'\nRemoving {len(removed)} objects within {REMOVE_R} of center:')
for r in removed: print(f'  #{r[0]:4d} id={r[1]:3d}(0x{r[1]:02x}) at ({r[2]:.2f},{r[3]:.2f})')
rmset={r[0] for r in removed}
keep=[recs[i] for i in range(n) if i not in rmset]

def mkrec(mid,x,y,z,rx,ry,rz,s):
    return struct.pack('<Hfffffff',mid,x*100.0,y*100.0,z,rx,ry,rz,s)

newrecs=list(keep)
for a in FENCE_ANGLES:
    t=math.radians(a); x=CX+R*math.cos(t); y=CY+R*math.sin(t)
    newrecs.append((FENCE_MODEL,x,y,mkrec(FENCE_MODEL,x,y,Z,0.0,0.0,a+90.0,1.0)))
for (x,y) in LIGHTS:
    newrecs.append((LIGHT_MODEL,x,y,mkrec(LIGHT_MODEL,x,y,Z,0.0,0.0,0.0,1.0)))

cnt=len(newrecs)
out=bytearray(d[:4]); out[2:4]=struct.pack('<H',cnt)
for _,_,_,rec in newrecs: out+=rec
assert len(out)==4+cnt*30
obj_new=enc(out)
open(f'{OUT}/EncTerrain1.obj','wb').write(obj_new)
print(f'\nOBJ objects: {n} -> {cnt}')
print(f'OBJ sha256: {hashlib.sha256(obj_new).hexdigest()}')
print(f'OBJ size: {len(obj_new)}')
print('OBJ roundtrip OK:', enc(dec(obj_new))==obj_new)

# ---------- VALIDATION ----------
g=bytes(new)
def blk(x,y):
    if not(0<=x<256 and 0<=y<256): return True
    return bool(g[y*256+x]&0x04)
xs=[c[0] for c in changed]; ys=[c[1] for c in changed]
print(f'\nDIFF bbox: X={min(xs)}..{max(xs)} Y={min(ys)}..{max(ys)} (allowed 137..145 / 124..132) ->',
      'OK' if (min(xs)>=137 and max(xs)<=145 and min(ys)>=124 and max(ys)<=132) else 'FAIL')
bad=[(x,y) for y in range(120,137) for x in range(133,150) if dist_of(x,y)<3.2 and blk(x,y)]
print(f'interior blocked cells (must be 0): {len(bad)} {bad[:5]}')

def bfs(start,goal,limit=200000):
    q=collections.deque([start]); seen={start}
    while q:
        x,y=q.popleft()
        if (x,y)==goal: return True,len(seen)
        for dx in (-1,0,1):
            for dy in (-1,0,1):
                if dx==0 and dy==0: continue
                nx,ny=x+dx,y+dy
                if (nx,ny) in seen or not(0<=nx<256 and 0<=ny<256): continue
                if blk(nx,ny): continue
                seen.add((nx,ny)); q.append((nx,ny))
                if len(seen)>limit: return False,len(seen)
    return False,len(seen)
print('BFS outside south (141,118) -> center:', bfs((141,118),(141,128)))
print('BFS outside north (141,140) -> center:', bfs((141,140),(141,128)))

q=collections.deque([(141,128)]); seen={(141,128)}
while q:
    x,y=q.popleft()
    for dx in (-1,0,1):
        for dy in (-1,0,1):
            if dx==0 and dy==0: continue
            nx,ny=x+dx,y+dy
            if (nx,ny) in seen or not(0<=nx<256 and 0<=ny<256): continue
            if dist_of(nx,ny)>=3.2 or blk(nx,ny): continue
            seen.add((nx,ny)); q.append((nx,ny))
need={(x,y) for y in range(120,137) for x in range(133,150) if dist_of(x,y)<3.0}
print(f'interior reachable from centre: {len(seen)} cells; missing needed={sorted(need-seen)}')

for name,a in [('E',0),('N',90),('W',180),('S',270)]:
    t=math.radians(a); ok=True; where=None
    for i in range(0,61):
        r=2.5+i*0.1
        x=int(round(CX+r*math.cos(t))); y=int(round(CY+r*math.sin(t)))
        if blk(x,y): ok=False; where=(x,y); break
    print(f'gate {name}: straight r=2.5..8.5 clear={ok}'+(f' blocked {where}' if where else ''))

sealed=bytearray(g)
for y in range(115,145):
    for x in range(127,160):
        i=y*256+x
        if 3.0<=dist_of(x,y)<=5.4 and near_gate(ang_of(x,y)): sealed[i]|=0x04
def blk2(x,y):
    if not(0<=x<256 and 0<=y<256): return True
    return bool(sealed[y*256+x]&0x04)
q=collections.deque([(0,0)]); seen2={(0,0)}
while q:
    x,y=q.popleft()
    for dx in (-1,0,1):
        for dy in (-1,0,1):
            if dx==0 and dy==0: continue
            nx,ny=x+dx,y+dy
            if (nx,ny) in seen2 or not(0<=nx<256 and 0<=ny<256): continue
            if blk2(nx,ny): continue
            seen2.add((nx,ny)); q.append((nx,ny))
interior={(x,y) for y in range(120,137) for x in range(133,150) if dist_of(x,y)<3.2}
print(f'LEAK TEST (gates sealed): outside flood={len(seen2)} cells, interior reached={len(interior&seen2)} (must be 0)')

prob=[]
for mid,px,py,_ in newrecs[-12:]:
    if blk(int(round(px)),int(round(py))): prob.append((mid,round(px,2),round(py,2)))
print(f'new object centres on blocked tile: {len(prob)} {prob}')
print(f'original records kept: {len(keep)} / {n-len(removed)}')

print('\nNEW ATT 132..152 x 120..137')
def chh(v):
    if v&4: return '#'
    if v&2: return 'n'
    if v&1: return 's'
    return '.'
print('      '+''.join(str(x%10) for x in range(132,153)))
for y in range(137,119,-1):
    print(f'{y:3d}   '+''.join(chh(g[y*256+x]) for x in range(132,153)))
print('\nlegend: s=safe walkable  #=blocked  n=nostand')

